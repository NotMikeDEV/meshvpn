
#include "globals.h"

#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

char tun_name[IFNAMSIZ] = {0};
char db_filename[1024];
void read_nodelist_file()
{
	if (db_filename[0])
	{
		FILE* fd;
		if( (fd = fopen(db_filename, "r")) > 0 )
		{
			if (Debug)
				printf("Reading from %s\n", db_filename);
			char Line[1024];
			memset(Line, 0, sizeof(Line));
			while (fgets(Line, sizeof(Line)-5, fd))
			{
				if (memcmp("node ", Line, 5) == 0)
				{
					char* IP = Line+5;
					char* Port = strchr(IP, ' ');
					if (Port)
					{
						Port[0]=0;
						Port++;
						char* END = strchr(Port, ' ');
						if (END)
							END[0]=0;
						
						struct sockaddr_in6 addr;
						addr.sin6_family = AF_INET6;
						if (inet_pton(AF_INET6, IP, &addr.sin6_addr) == 1 && atoi(Port) && atoi(Port) <= 65535)
						{
							addr.sin6_port = htons(atoi(Port));
							struct Node* Node = node_add(&addr);
							if (Debug)
							{
								printf("Loaded node %s %s", IP, Port);
							}
						}
					}
				}
				memset(Line, 0, sizeof(Line));
			}
		}
	}
}
void write_nodelist_file()
{
	if (db_filename[0])
	{
		int fd;
		if( (fd = open(db_filename, O_RDWR|O_CREAT|O_TRUNC)) < 0 )
		{
			perror("Error writing node cache");
		}
		else
		{
			if (Debug)
				printf("Writing to %s\n", db_filename);
			struct Node* Node = FirstNode;
			while (Node)
			{
				char IP[72];
				char Line[512];
				char NodeKey[sizeof(node_key)*2+1];
				for (int x=0; x<sizeof(node_key); x++)
				{
					sprintf(NodeKey+(x*2), "%02x", Node->Key[x]);
				}
				inet_ntop(AF_INET6, &Node->address.sin6_addr, IP, sizeof(IP));
				sprintf(Line, "node %s %u %s %u\n", IP, htons(Node->address.sin6_port), NodeKey, Node->State);
				write(fd, Line, strlen(Line));
				if (Debug)
					printf("%s", Line);
				Node=Node->Next;
			}
			struct EthernetNode* EthernetNode = FirstEthernetNode;
			while (EthernetNode)
			{
				char Line[512];
				char NodeKey[sizeof(node_key)*2+1];
				for (int x=0; x<sizeof(node_key); x++)
				{
					sprintf(NodeKey+(x*2), "%02x", EthernetNode->Node->Key[x]);
				}
				char MAC[13];
				for (int x=0; x<6; x++)
				{
					sprintf(MAC+(x*2), "%02x", EthernetNode->MAC[x]);
				}
				sprintf(Line, "MAC %s %s\n", MAC, NodeKey);
				write(fd, Line, strlen(Line));
				if (Debug)
					printf("%s", Line);
				EthernetNode=EthernetNode->Next;
			}
			close(fd);
		}
	}
}
int tun_alloc(char *dev)
{
	struct ifreq ifr;
	int fd, err;

	if( (fd = open("/dev/net/tun", O_RDWR)) < 0 )
		return -1;

	memset(&ifr, 0, sizeof(ifr));

	/* Flags: IFF_TUN   - TUN device (no Ethernet headers) 
	 *        IFF_TAP   - TAP device  
	 *
	 *        IFF_NO_PI - Do not provide packet information  
	 */ 
	ifr.ifr_flags = IFF_TAP; 
	if( *dev )
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);

	if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 )
	{
		close(fd);
		return err;
	}
	strcpy(dev, ifr.ifr_name);
	interface_name = dev;
	return fd;
}
void print_usage_and_exit(char* cmd)
{
	printf("Usage: %s <port> <netpass> [options]\n", cmd);
	printf("\t<port>             UDP Port to listen on.\n");
	printf("\t<netpass>          Network Password. (must be the same on all nodes)\n");
	printf("Options:\n");
	printf("\t-c <filename>      Filename of node database.\n");
	printf("\t-h <IP> <Port>     Specify an init node. (can be used multiple times)\n");
	printf("\t-i <name>          Specify interface name.\n");
	printf("\t-4 <0.0.0.0[/24]>  Specify IPv4 address.\n");
	printf("\t-6 <::0[/64]>      Specify IPv6 address.\n");
	printf("\t-bird <bird_conf>  Specify file to write bird.conf to.\n");
	printf("\t-bird6 <bird_conf> Specify file to write bird6.conf to.\n");
	printf("\t-pcap <filename>   Capture packets to filename. [pcap format] (debug)\n");
	printf("\t-D                 Debug Output.\n");
	exit(1);
}
int main(int argc, char** argv)
{
	BirdConf=NULL;
	memset(network_key, 0, sizeof(network_key));
	pcap_file = -1;
	char tun_name[IFNAMSIZ] = {0};
	memset(&node_IPv4, 0, sizeof(node_IPv4));
	memset(&node_IPv6, 0, sizeof(node_IPv6));
	uint16_t Port = 0;
	int RoutingTable=-1;
	char** argvp = argv;
	Debug=0;
	memset(db_filename, 0, sizeof(db_filename));
	while (*++argvp)
	{
		if (strcmp(*argvp, "-D") == 0)
		{
			Debug++;
		}
		else if (strcmp(*argvp, "-i") == 0)
		{
			if (*(argvp[1]))
			{
				strncpy(tun_name, argvp[1], sizeof(tun_name));
				argvp++;
			}
			else
			{
				print_usage_and_exit(argv[0]);
			}
		}
		else if (strcmp(*argvp, "-4") == 0)
		{
			int len=strlen(argvp[1]);
			for (int x=0; x<len; x++)
			{
				if (argvp[1][x] == '/')
				{
					argvp[1][x]=0;
					sscanf(&argvp[1][x+1], "%hu", &node_IPv4_pl);
					break;
				}
			}
			if (*(argvp[1]) && inet_pton(AF_INET, argvp[1], &node_IPv4) && node_IPv4_pl && node_IPv4_pl <= 32)
			{
				argvp++;
			}
			else
			{
				printf("Invalid IPv4 Prefix: %s/%u\n", argvp[1], node_IPv4_pl);
				print_usage_and_exit(argv[0]);
			}
		}
		else if (strcmp(*argvp, "-6") == 0)
		{
			int len=strlen(argvp[1]);
			for (int x=0; x<len; x++)
			{
				if (argvp[1][x] == '/')
				{
					argvp[1][x]=0;
					sscanf(&argvp[1][x+1], "%hu", &node_IPv6_pl);
					break;
				}
			}
			if (*(argvp[1]) && inet_pton(AF_INET6, argvp[1], &node_IPv6) && node_IPv6_pl && node_IPv6_pl <= 128)
			{
				argvp++;
			}
			else
			{
				printf("Invalid IPv6 Prefix: %s/%u\n", argvp[1], node_IPv6_pl);
				print_usage_and_exit(argv[0]);
			}
		}
		else if (db_filename[0] && strcmp(*argvp, "-h") == 0)
		{
			if (argvp[1] && *(argvp[1]) && argvp[2] && *(argvp[2]))
			{
				char* IP = argvp[1];
				int Port = atol(argvp[2]);

				struct sockaddr_in6 addr;
				addr.sin6_family = AF_INET6;
				if (inet_pton(AF_INET6, IP, &addr.sin6_addr) == 1 && Port && Port <= 65535)
				{
					addr.sin6_port = htons(Port);
					struct Node* Node = node_add(&addr);
                                        Node->Static=1;
					if (Debug)
					{
						printf("Added IPv6 node %s %d", IP, Port);
					}
				}
				else
				{
					struct sockaddr_in addr4;
					if (inet_pton(AF_INET, IP, &addr4.sin_addr) == 1 && Port && Port <= 65535)
					{
						char IPv6[128];
						char IPv4[32];
						inet_ntop(AF_INET, &addr4.sin_addr, IPv4, sizeof(IPv4));
						sprintf(IPv6, "::ffff:%s", IPv4);
						inet_pton(AF_INET6, IPv6, &addr.sin6_addr);
						addr.sin6_port = htons(Port);
						struct Node* Node = node_add(&addr);
                                                Node->Static=1;
						if (Debug)
						{
							inet_ntop(AF_INET6, &addr.sin6_addr, IPv6, sizeof(IP));
							printf("Added IPv4 node %s %d", IPv6, Port);
						}
					}
				}
			}
			else
			{
				printf("Invalid node format. Try IP Port\n");
				print_usage_and_exit(argv[0]);
			}
			argvp++;
			argvp++;
		}
		else if (strcmp(*argvp, "-bird") == 0)
		{
			BirdConf = argvp[1];
			if (!BirdConf)
				print_usage_and_exit(argv[0]);
			argvp++;
		}
		else if (strcmp(*argvp, "-bird6") == 0)
		{
			Bird6Conf = argvp[1];
			if (!Bird6Conf)
				print_usage_and_exit(argv[0]);
			argvp++;
		}
		else if (strcmp(*argvp, "-pcap") == 0)
		{
			if (*(argvp[1]))
			{
				if( (pcap_file = open(argvp[1], O_RDWR|O_CREAT|O_TRUNC)) < 0 )
				{
					perror("Error opening pcap file.");
					exit(-1);
				}
				struct pcap_hdr header={0};
				header.magic_number = 0xa1b2c3d4;
				header.version_major = 2;
				header.version_minor = 4;
				header.thiszone = 0;
				header.sigfigs = 0;
				header.snaplen = 65535;
				header.network = 1;
				write(pcap_file, &header, sizeof(header));
				argvp++;
			}
			else
			{
				print_usage_and_exit(argv[0]);
			}
		}
		else if (strcmp(*argvp, "-c") == 0)
		{
			if (*(argvp[1]))
				strcpy(db_filename, argvp[1]);
			else
				print_usage_and_exit(argv[0]);
			argvp++;
		}
		else if (!Port)
		{
			Port = atoi(*argvp);
			if (!Port)
			{
				printf("Invalid Port: %s\n", *argvp);
				print_usage_and_exit(argv[0]);
			}
			init_network(Port);
		}
		else if (!network_key[0] && !network_key[1] && !network_key[2] && !network_key[4])
		{
			init_crypto(*argvp);
		}
		else
		{
			printf("Unknown argument: %s\n", *argvp);
			print_usage_and_exit(argv[0]);
		}
	}
	if ((!network_key[0] && !network_key[1] && !network_key[2] && !network_key[4]) || !Port)
	{
		print_usage_and_exit(argv[0]);
	}
	read_nodelist_file();
	
	tun_fd = tun_alloc(tun_name);  /* tun interface */
	int status=0;
	if (fork() == 0) execlp("ip", "ip", "link", "set", tun_name, "up", NULL); else wait(&status);
	if (inet_netof(node_IPv4) != INADDR_ANY)
	{
		char IPv4[128];
		inet_ntop(AF_INET, &node_IPv4, IPv4, sizeof(IPv4));
		sprintf(IPv4+strlen(IPv4), "/%d", node_IPv4_pl);
		if (fork() == 0) execlp("ip", "ip", "addr", "add", IPv4, "dev", tun_name, NULL); else wait(&status);
		if (Debug)
			printf("IPv4: %s\n", IPv4);
	}
	if (memcmp(&node_IPv6, &in6addr_any, sizeof(struct in6_addr)))
	{
		char IPv6[128];
		inet_ntop(AF_INET6, &node_IPv6, IPv6, sizeof(IPv6));
		sprintf(IPv6+strlen(IPv6), "/%d", node_IPv6_pl);
		if (fork() == 0) execlp("ip", "ip", "-6", "addr", "add", IPv6, "dev", tun_name, NULL); else wait(&status);
		if (Debug)
			printf("IPv6: %s\n", IPv6);
	}
	
	if(tun_fd < 0){
	    perror("Allocating interface");
	    exit(1);
	}
	if (Debug)
		printf("Interface %s\n", tun_name);
	if (BirdConf)
		init_router();
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(tun_fd, &read_fds);
	FD_SET(network_socket, &read_fds);
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	time_t lastrefresh = 0;
	int FD_MAX = MAX(tun_fd, network_socket);
	while(1)
	{
		select(FD_MAX + 1, &read_fds, NULL, NULL, &timeout);
		unsigned char buffer[10000];
		if (time(NULL) - lastrefresh >= 5)
		{
			network_send_init_packets();
			network_send_nodelists();
			network_purge_nodelist();
			write_nodelist_file();
			lastrefresh = time(NULL);
		}
		if (FD_ISSET(tun_fd, &read_fds))
		{
			int nread = read(tun_fd,buffer,sizeof(buffer));
			if(nread < 0)
			{
				perror("Reading from interface");
				exit(1);
			}

			if (Debug>1)
				printf("Read %d bytes from device %s\n", nread-4, tun_name);
			struct pcaprec_hdr header = {0};
			struct timeval tv={0};
			gettimeofday(&tv, NULL);
			header.ts_sec = tv.tv_sec;
			header.ts_usec = tv.tv_usec;
			header.incl_len=nread-4;
			header.orig_len=nread-4;
			write(pcap_file, &header, sizeof(header));
			write(pcap_file, buffer+4, nread-4);
			network_send_ethernet_packet(buffer, nread);
		}
		if (FD_ISSET(network_socket, &read_fds))
		{
			struct sockaddr_in6 addr;
			int addrlen = sizeof(addr);
			int nread = recvfrom(network_socket,buffer,sizeof(buffer), MSG_DONTWAIT, (struct sockaddr *)&addr, &addrlen);
			if(nread < 0)
			{
				perror("Reading from socket");
				exit(1);
			}
			char packet[sizeof(buffer)];
			int decrypted_len = decrypt_packet(buffer, nread, packet);
			network_parse(packet, decrypted_len, &addr);
		}
		FD_ZERO(&read_fds);
		FD_SET(tun_fd, &read_fds);
		FD_SET(network_socket, &read_fds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
	}
}
