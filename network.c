#include "globals.h"

void init_network(uint16_t UDPPort)
{
	int fd;
	if( (fd = open("/dev/urandom", O_RDONLY)) < 0 )
	{
		perror("open /dev/urandom");
		exit(-1);
	}
	read(fd, node_key, sizeof(node_key));
	close(fd);
	if (Debug)
	{
		printf("Node Key: ");
		for (int i = 0; i < sizeof(node_key); i++) {
			printf("%02x", node_key[i]);
		}
		printf("\n");
	}
	
	socklen_t clilen;
	struct sockaddr_in6 server_addr;
	char buffer[2048];
	network_socket = socket(PF_INET6, SOCK_DGRAM, 0);
	if (network_socket < 0) {
		perror("creating socket");
		exit(-1);
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin6_family = AF_INET6;
	server_addr.sin6_port = htons(UDPPort);

	if (bind(network_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("bind failed");
		exit(-1);
	}
}
void send_packet(struct Node* to, unsigned char type, unsigned char* packet, int len)
{
	unsigned char* full_packet=malloc(sizeof(node_key)*2 + 1 + len);
	full_packet[0]=type;
	memcpy(full_packet+1, node_key, sizeof(node_key));
	memcpy(full_packet+1+sizeof(node_key), to->Key, sizeof(node_key));
	if (len)
		memcpy(full_packet+1+(sizeof(node_key)*2), packet, len);
	unsigned char* ciphertext = malloc(sizeof(node_key)*2 + 1 + len + 16);
	int cipherlen = encrypt_packet(full_packet, sizeof(node_key)*2 + 1 + len, ciphertext);
	sendto(network_socket, ciphertext, cipherlen, 0, (struct sockaddr *)&to->address, sizeof(struct sockaddr_in6));
	to->LastSend = time(NULL);
}
void network_send_ethernet_packet(unsigned char* buffer, int len)
{
	struct Node* Node = node_find_mac(buffer+4);
	if (Node && Node->State == 1)
	{
		char IP[74];
		inet_ntop(AF_INET6, &Node->address.sin6_addr, IP, sizeof(IP));
		if (Debug>1)
			printf("Sent packet for %02X%02X%02X%02X%02X%02X to %s %02X%02X%02X%02X%02X%02X\n", buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[9], IP,
				Node->Key[0], Node->Key[1], Node->Key[2], Node->Key[3], Node->Key[4], Node->Key[5]);
		send_packet(Node, 0xFF, buffer, len);
		return;
	}
	Node = FirstNode;
	while (Node)
	{
		if (Node->State == 1)
		{
			if (Debug>1)
			{
				char IP[74];
				inet_ntop(AF_INET6, &Node->address.sin6_addr, IP, sizeof(IP));
				printf("Broadcasting packet for %02X%02X%02X%02X%02X%02X to %s %02X%02X%02X%02X%02X%02X\n", buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[9], IP,
					Node->Key[0], Node->Key[1], Node->Key[2], Node->Key[3], Node->Key[4], Node->Key[5]);
			}
			send_packet(Node, 0xFF, buffer, len);
		}
		Node = Node->Next;
	}
}
void network_parse(unsigned char* packet, int length, struct sockaddr_in6* addr)
{
	char IP[128];
	inet_ntop(AF_INET6, &addr->sin6_addr, IP, sizeof(IP));
	unsigned char* from_key = packet+1;
	unsigned char* to_key = packet+1+sizeof(node_key);
	unsigned char* payload = packet+1+(sizeof(node_key)*2);
	int payload_length = length - 1 - (sizeof(node_key)*2);
	if (memcmp(from_key, node_key, sizeof(node_key)) == 0)
		return;
	switch (packet[0])
	{
	case 0x00:
	{	if (Debug)
		{
			printf("Got INIT packet from ");
			for (int x=0; x<sizeof(node_key); x++)
			{
				printf("%02x", from_key[x]);
			}
			printf(" (%s %u)\n", IP, htons(addr->sin6_port));
		}
		struct Node* Node = node_add(addr);
		memcpy(Node->Key, from_key, sizeof(node_key));
		char Addresses[sizeof(struct in_addr) + sizeof(struct in6_addr)];
		memcpy(Addresses, &node_IPv4, sizeof(struct in_addr));
		memcpy(Addresses+4, &node_IPv6, sizeof(struct in6_addr));
		send_packet(Node, 0x01, Addresses, sizeof(Addresses));
	}	break;
	case 0x01:
	{
		if (memcmp(to_key, node_key, sizeof(node_key)))
			return;
		struct Node* Node = node_find_addr(addr);
		if (!Node)
			return;
		memcpy(&Node->Key, from_key, sizeof(node_key));
		memcpy(&Node->IPv4, payload, 4);
		memcpy(&Node->IPv6, payload+4, 16);
		Node->State=1;
                Node->LastRecv = time(NULL);
		if (Debug)
		{
			char remote_key[sizeof(node_key)*2+1];
			memset(remote_key, 0, sizeof(remote_key));
			for (int x=0; x<sizeof(node_key); x++)
			{
				sprintf(remote_key+(x*2), "%02x", from_key[x]);
			}
			char IPv4[16];
			char IPv6[72];
			inet_ntop(AF_INET, &Node->IPv4, IPv4, sizeof(IPv4));
			inet_ntop(AF_INET6, &Node->IPv6, IPv6, sizeof(IPv6));
			printf("Got INIT REPLY packet from %s (%s %u)\n", remote_key, IP, htons(addr->sin6_port));
			printf("Node IPv4: %s, IPv6: %s\n", IPv4, IPv6);
		}

		char Addresses[sizeof(struct in_addr) + sizeof(struct in6_addr)];
		memcpy(Addresses, &node_IPv4, sizeof(struct in_addr));
		memcpy(Addresses+4, &node_IPv6, sizeof(struct in6_addr));
		send_packet(Node, 0x02, Addresses, sizeof(Addresses));
	}	break;
	case 0x02:
	{
		if (memcmp(to_key, node_key, sizeof(node_key)))
			return;
		struct Node* Node = node_find_addr(addr);
		if (!Node)
			return;
		if (Debug)
		{
			char remote_key[sizeof(node_key)*2+1];
			for (int x=0; x<sizeof(node_key); x++)
			{
				sprintf(remote_key+(x*2), "%02x", from_key[x]);
			}
			char IPv4[16];
			char IPv6[72];
			inet_ntop(AF_INET, packet+1+sizeof(node_key)*2, IPv4, sizeof(IPv4));
			inet_ntop(AF_INET6, packet+1+sizeof(node_key)*2+4, IPv6, sizeof(IPv6));
			printf("Got INIT ACK packet from %s (%s %u)\n", remote_key, IP, htons(addr->sin6_port));
			printf("Node IPv4: %s, IPv6: %s\n", IPv4, IPv6);
		}
		memcpy(&Node->Key, from_key, sizeof(node_key));
		memcpy(&Node->IPv4, payload, 4);
		memcpy(&Node->IPv6, payload+4, 16);
		Node->State=1;
                Node->LastRecv = time(NULL);
	}	break;
	case 0x03:
	{
		if (memcmp(to_key, node_key, sizeof(node_key)))
			return;
		struct Node* Node = node_find_key(from_key);
		if (!Node)
		{
			node_add(addr);
			return;
		}
		unsigned char count=payload[0];
		if (Debug)
		{
			char remote_key[sizeof(node_key)*2+1];
			for (int x=0; x<sizeof(node_key); x++)
			{
				sprintf(remote_key+(x*2), "%02x", from_key[x]);
			}
			printf("Got nodelist packet from ");
			printf("%s (%s %u)\n", remote_key, IP, htons(addr->sin6_port));
			printf("Count: %u\n", count);
		}
		for (int x=0; x<count; x++)
		{
			struct NodeEntry* Entry=(struct NodeEntry*)(payload+1+(20*x));
			if (Debug > 1)
			{
				char IPr[1024];
				inet_ntop(AF_INET6, &Entry->address, IPr, sizeof(IPr));
				printf("IP: %s, Port: %u\n", IPr, htons(Entry->port));
			}
			struct sockaddr_in6 address;
			memset(&address, 0, sizeof(address));
			address.sin6_family = AF_INET6;
			memcpy(&address.sin6_addr, &Entry->address, sizeof(Entry->address));
			address.sin6_port = Entry->port;
			node_add(&address);
		}
	}	break;
	case 0xF0:
	{
		if (memcmp(to_key, node_key, sizeof(node_key)))
			return;
		if (Debug)
		{
			char remote_key[sizeof(node_key)*2+1];
			for (int x=0; x<sizeof(node_key); x++)
			{
				sprintf(remote_key+(x*2), "%02x", packet[1+x]);
			}
			printf("Got PING packet from %s (%s %u)\n", remote_key, IP, htons(addr->sin6_port));
		}
		struct Node* Node = node_find_key(from_key);
		if (!Node)
		{
			node_add(addr);
			return;
		}
		Node->LastRecv=time(NULL);
		send_packet(Node, 0xF1, NULL, 0);
	}	break;
	case 0xF1:
	{
		if (memcmp(to_key, node_key, sizeof(node_key)))
			return;
		struct Node* Node = node_find_key(from_key);
		if (!Node)
		{
			return;
		}
		Node->LastRecv=time(NULL);
		struct timeval ReplyTime={0};
		gettimeofday(&ReplyTime, NULL);
		struct timeval diff={0};
		diff.tv_sec = ReplyTime.tv_sec - Node->LastPing.tv_sec;
		diff.tv_usec = ReplyTime.tv_usec - Node->LastPing.tv_usec;
		if (diff.tv_usec < 1)
		{
			diff.tv_usec+=1000000;
			diff.tv_sec-=1;
		}
		Node->Latency = diff.tv_sec*1000 + diff.tv_usec/1000;
		if (Debug)
		{
			char remote_key[sizeof(node_key)*2+1];
			for (int x=0; x<sizeof(node_key); x++)
			{
				sprintf(remote_key+(x*2), "%02x", packet[1+x]);
			}
			printf("Got PING REPLY packet from %s (%s %u)\n", remote_key, IP, htons(addr->sin6_port));
			printf("Latency: %dms\n", Node->Latency);
		}
		update_router();
	}	break;
	case 0xFF:
	{
		if (memcmp(to_key, node_key, sizeof(node_key)))
			return;
		struct Node* Node = node_find_mac(payload + 4+6);
		if (!Node)
		{
			Node = node_find_key(from_key);
			if (Node)
				node_add_mac(Node, payload+4+6);
			else
			{
				if (Debug>1)
				{
					char remote_key[sizeof(node_key)*2+1];
					for (int x=0; x<sizeof(node_key); x++)
					{
						sprintf(remote_key+(x*2), "%02x", from_key[x]);
					}
					printf("Got INVALID %ub packet from %s ", payload_length,remote_key);
					printf("(%s %u)\n", IP, htons(addr->sin6_port));
				}
				return;
			}
		}
		Node->LastRecv = time(NULL);
		if (Debug>1)
		{
			char remote_key[sizeof(node_key)*2+1];
			for (int x=0; x<sizeof(node_key); x++)
			{
				sprintf(remote_key+(x*2), "%02x", from_key[x]);
			}
			printf("Got %ub packet from %s", payload_length, remote_key);
			printf(" (%s %u) DST:%02X%02X%02X%02X%02X%02X SRC:%02X%02X%02X%02X%02X%02X\n", IP, htons(Node->address.sin6_port),
				payload[4 + 0], payload[4 + 1], payload[4 + 2], payload[4 + 3], payload[4 + 4], payload[4 + 5],
				payload[4 + 6], payload[4 + 7], payload[4 + 8], payload[4 + 9], payload[4 + 10], payload[4 + 11]);
		}
		write(tun_fd, payload, payload_length);
		if (pcap_file != -1)
		{
			struct pcaprec_hdr header = {0};
			struct timeval tv={0};
			gettimeofday(&tv, NULL);
			header.ts_sec = tv.tv_sec;
			header.ts_usec = tv.tv_usec;
			header.incl_len=payload_length - 4;
			header.orig_len=payload_length - 4;
			write(pcap_file, &header, sizeof(header));
			write(pcap_file, payload + 4, payload_length - 4);
		}
	}	break;
	default:
		break;
	}
}
void network_ping(struct Node* Node)
{
	if (Debug)
	{
		printf("Sending PING to ");
		for (int x=0; x<sizeof(node_key); x++)
		{
			printf("%02x", Node->Key[x]);
		}
		char IP[128];
		inet_ntop(AF_INET6, &Node->address.sin6_addr, IP, sizeof(IP));
		printf(" (%s %u)\n", IP, htons(Node->address.sin6_port));
	}
	gettimeofday(&Node->LastPing, NULL);
	send_packet(Node, 0xF0, NULL, 0);
}
void network_send_pings()
{
	struct Node* Node = FirstNode;
	while (Node)
	{
		if (Node->State && Node->LastPing.tv_sec < time(NULL) - 60)
		{
			network_ping(Node);
		}
		Node = Node->Next;
	}
}
void network_send_init_packets()
{
	struct Node* Node = FirstNode;
	while(Node)
	{
		if (Node->State == 0)
		{
			Node->Retries++;
			send_packet(Node, 0x00, NULL, 0);
			if (Debug)
			{
				char IP[128];
				inet_ntop(AF_INET6, &Node->address.sin6_addr, IP, sizeof(IP));
				printf("Sent INIT packet to %s:%u\n", IP, htons(Node->address.sin6_port));
			}
		}
		Node=Node->Next;
	}
}
void network_send_nodelist(struct Node* node)
{
	if (Debug)
	{
		printf("Sending node list to %02x%02x%02x%02x%02x%02x\n", node->Key[0], node->Key[1], node->Key[2], node->Key[3], node->Key[4], node->Key[5]);
	}
	unsigned char nodelist[sizeof(struct NodeEntry)*NODE_LIST_MAX + 1];
	memset(nodelist, 1, sizeof(nodelist));
	unsigned char count=0;
	struct Node* Node = FirstNode;
	while (Node)
	{
		if (count >= NODE_LIST_MAX)
		{
			nodelist[0] = count;
			send_packet(node, 0x03, nodelist, 1+count*20);
			count=0;
		}
		struct NodeEntry* Entry = (struct NodeEntry*)(nodelist+1+(20*count));
		memcpy(&Entry->address, &Node->address.sin6_addr, sizeof(Entry->address));
		Entry->port=Node->address.sin6_port;
		count++;
		Node = Node->Next;
	}
	nodelist[0] = count;
	send_packet(node, 0x03, nodelist, 1+count*20);
	node->LastSendNodeList = time(NULL);
}
void network_send_nodelists()
{
	struct Node* Node = FirstNode;
	while(Node)
	{
		if (Node->State == 1 && time(NULL) - Node->LastSendNodeList > 300)
		{
			network_send_nodelist(Node);
		}
		Node=Node->Next;
	}
}
void network_purge_nodelist()
{
	int connected_count=0;
	struct Node* Node = FirstNode;
	while(Node)
	{
		if (Node->State == 1)
			connected_count++;
		Node=Node->Next;
	}
	Node = FirstNode;
	while(Node)
	{
		if (Node->State == 1 && time(NULL) - Node->LastRecv > 90)
		{
			Node->State = 0;
			Node->Retries = 0;
			node_delete_macs(Node);
		}
		else 
		{
			if (Node->Retries > 5 && connected_count)
			{
				struct Node* Next = Node->Next;
				node_delete(Node);
				Node=Next;
				continue;
			}	
		}
		Node=Node->Next;
	}
}