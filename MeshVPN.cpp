#include "Globals.h"
MeshVPN app;

MeshVPN::MeshVPN()
{
	Running = true;
}
void MeshVPN::Init(Switch* Switch)
{
	this->LocalSwitch = Switch;
}
void MeshVPN::AddSocket(SocketHandler* Handler)
{
	SocketList.push_back(Handler);
}
void MeshVPN::DelSocket(SocketHandler* Handler)
{
	for (int index = 0; index < SocketList.size(); ++index)
	{
		if (SocketList[index] == Handler)
		{
			SocketList.erase(SocketList.begin() + index);
			return;
		}
	}
}
bool MeshVPN::Loop()
{
	fd_set read_fds;
	FD_ZERO(&read_fds);
	fd_set write_fds;
	FD_ZERO(&write_fds);
	fd_set except_fds;
	FD_ZERO(&except_fds);
	int FD_MAX = 0;
	for (SocketHandler* Handler : SocketList)
	{
		int fd = Handler->GetHandle();
		FD_SET(fd, &read_fds);
		FD_SET(fd, &except_fds);
		if (Handler->WriteBufferLength)
			FD_SET(fd, &write_fds);
		FD_MAX = MAX(FD_MAX, fd);
	}
	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	select(FD_MAX + 1, &read_fds, &write_fds, &except_fds, &timeout);
	for (int index = 0; index < SocketList.size(); ++index)
	{
		int fd = SocketList[index]->GetHandle();
		if (FD_ISSET(fd, &read_fds))
		{
			while (SocketList[index]->ReadHandle());
		}
		if (FD_ISSET(fd, &write_fds))
		{
			SocketList[index]->WriteHandle();
		}
	}
	LocalSwitch->RunInterval();
	return Running;
}
void MeshVPN::SaveNodes(char* Filename)
{
	int fd;
	if ((fd = open(Filename, O_RDWR | O_CREAT | O_TRUNC, 0664)) < 0)
	{
		perror("Error writing node cache");
	}
	else
	{
		if (Debug)
			printf("Writing to %s\n", Filename);
		for (RemoteNode* n : LocalSwitch->RemoteNodes)
		{
			char IP[72];
			char Line[512];
			char NodeKey[NODE_KEY_LENGTH * 2 + 1];
			for (int x = 0; x < NODE_KEY_LENGTH; x++)
			{
				sprintf(NodeKey + (x * 2), "%02x", n->NodeKey[x]);
			}
			inet_ntop(AF_INET6, &n->Address.sin6_addr, IP, sizeof(IP));
			sprintf(Line, "node %s %u %s %u %ums\n", IP, htons(n->Address.sin6_port), NodeKey, n->GetStatus(), n->Latency);
			write(fd, Line, strlen(Line));
			for (struct MAC &E : n->MACs)
			{
				sprintf(Line, "MAC %02X%02X%02X%02X%02X%02X\n", E.a[0], E.a[1], E.a[2], E.a[3], E.a[4], E.a[5]);
				write(fd, Line, strlen(Line));
			}
			if (Debug>1)
				printf("%s", Line);
		}
	}
}
void MeshVPN::LoadNodes(char* Filename)
{
	FILE* fd;
	if ((fd = fopen(Filename, "r")) > 0)
	{
		if (Debug)
			printf("Reading from %s\n", Filename);
		char Line[1024];
		memset(Line, 0, sizeof(Line));
		while (fgets(Line, sizeof(Line) - 5, fd))
		{
			if (memcmp("node ", Line, 5) == 0)
			{
				char* IP = Line + 5;
				char* Port = strchr(IP, ' ');
				if (Port)
				{
					Port[0] = 0;
					Port++;
					char* END = strchr(Port, ' ');
					if (END)
						END[0] = 0;

					struct sockaddr_in6 addr;
					addr.sin6_family = AF_INET6;
					if (inet_pton(AF_INET6, IP, &addr.sin6_addr) == 1 && atoi(Port) && atoi(Port) <= 65535)
					{
						addr.sin6_port = htons(atoi(Port));
						LocalSwitch->AddRemote(&addr, false);
					}
				}
			}
			memset(Line, 0, sizeof(Line));
		}
	}
}
MeshVPN::~MeshVPN()
{
	if (Debug)
		printf("Exit\n");
}

void term(int signum)
{
	app.Running=false;
}
void print_usage_and_exit(char* cmd)
{
	printf("Usage: %s [options]\n", cmd);
	printf("\t-C <filename>      Specify file to save node cache to.\n");
	printf("\t-i <name>          Specify interface name.\n");
	printf("\t-p <port>          Specify UDP/TCP port.\n");
	printf("\t-P <password>      Specify network password.\n");
	printf("\t-4 <IP/Prefix>     Specify IPv4 Address.\n");
	printf("\t-6 <IP/Prefix>     Specify IPv6 Address.\n");
	printf("\t-R <tableno>       Enable routing synchronisation.\n");
	printf("\t-D                 Increase debug level (use multiple times).\n");
	exit(1);
}
unsigned char Debug;
int main(int argc, char* argv[])
{
	Debug = 0;
	Switch Switch(&app);
	char node_filename[1024];
	node_filename[0] = 0;
	char tun_name[IFNAMSIZ] = { 0 };
	int Port = 0;
	int RoutingTable = -1;
	unsigned char network_key[32];
	memset(network_key, 0, sizeof(network_key));
	char** argvp = argv;
	while (*++argvp)
	{
		if (strcmp(*argvp, "-C") == 0)
		{
			if (argvp[1] && *(argvp[1]))
			{
				strncpy(node_filename, argvp[1], sizeof(node_filename));
				argvp++;
			}
			else
			{
				printf("Usage: -C <filename>\n");
				print_usage_and_exit(argv[0]);
			}
		}
		else if (strcmp(*argvp, "-i") == 0)
		{
			if (argvp[1] && *(argvp[1]))
			{
				strncpy(tun_name, argvp[1], sizeof(tun_name));
				argvp++;
			}
			else
			{
				printf("Usage: -i <name>\n");
				print_usage_and_exit(argv[0]);
			}
		}
		else if (strcmp(*argvp, "-p") == 0)
		{
			if (argvp[1] && *(argvp[1]))
			{
				Port = atoi(argvp[1]);
				if (!Port)
				{
					printf("Invalid Port: %s\n", argvp[1]);
					print_usage_and_exit(argv[0]);
				}
				argvp++;
			}
			else
			{
				printf("Usage: -p <port>\n");
				print_usage_and_exit(argv[0]);
			}
		}
		else if (strcmp(*argvp, "-P") == 0)
		{
			if (argvp[1] && *(argvp[1]))
			{
				SHA256((const unsigned char*)argvp[1], strlen(argvp[1]), network_key);
				argvp++;
			}
			else
			{
				printf("Usage: -P <password>\n");
				print_usage_and_exit(argv[0]);
			}
		}
		else if (strcmp(*argvp, "-4") == 0)
		{
			struct in_addr node_IPv4;
			short node_IPv4_pl=-1;
			int len = strlen(argvp[1]);
			for (int x = 0; x < len; x++)
			{
				if (argvp[1][x] == '/')
				{
					argvp[1][x] = 0;
					sscanf(&argvp[1][x + 1], "%hu", &node_IPv4_pl);
					break;
				}
			}
			if (*(argvp[1]) && inet_pton(AF_INET, argvp[1], &node_IPv4) && node_IPv4_pl && node_IPv4_pl <= 32)
			{
				Switch.SetIPv4(&node_IPv4, node_IPv4_pl);
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
			struct in6_addr node_IPv6;
			short node_IPv6_pl=-1;
			int len = strlen(argvp[1]);
			for (int x = 0; x < len; x++)
			{
				if (argvp[1][x] == '/')
				{
					argvp[1][x] = 0;
					sscanf(&argvp[1][x + 1], "%hu", &node_IPv6_pl);
					break;
				}
			}
			if (*(argvp[1]) && inet_pton(AF_INET6, argvp[1], &node_IPv6) && node_IPv6_pl && node_IPv6_pl <= 128)
			{
				Switch.SetIPv6(&node_IPv6, node_IPv6_pl);
				argvp++;
			}
			else
			{
				printf("Invalid IPv6 Prefix: %s/%u\n", argvp[1], node_IPv6_pl);
				print_usage_and_exit(argv[0]);
			}
		}
		else if (strcmp(*argvp, "-R") == 0)
		{
			if (argvp[1] && *(argvp[1]))
			{
				RoutingTable = atoi(argvp[1]);
				if (RoutingTable < 0 || RoutingTable > 255)
				{
					printf("Invalid Routing Table: %s\n", argvp[1]);
					print_usage_and_exit(argv[0]);
				}
				argvp++;
			}
			else
			{
				printf("Usage: -R <tableno>\n");
				print_usage_and_exit(argv[0]);
			}
		}
		else if (strcmp(*argvp, "-h") == 0)
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
					Switch.AddRemote(&addr, true);
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
						Switch.AddRemote(&addr, true);
					}
					else
					{
						printf("Invalid node format. Try IP Port\n");
						print_usage_and_exit(argv[0]);
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
		else if (strcmp(*argvp, "-D") == 0)
		{
			Debug++;
		}
		else
		{
			printf("Unknown command line option: %s\n", argvp[0]);
			print_usage_and_exit(argv[0]);
		}
	}
	Crypto::Init(network_key);
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT, &action, NULL);

	Switch.Init(tun_name, Port, RoutingTable);
	if (node_filename[0])
	{
		app.LoadNodes(node_filename);
	}
	time_t LastSave = time(NULL);
	while (app.Loop())
	{
		if (node_filename[0] && time(NULL) - LastSave > 20)
		{
			app.SaveNodes(node_filename);
			LastSave = time(NULL);
		}
	}
	return 0;
}