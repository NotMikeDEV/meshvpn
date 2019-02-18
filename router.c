#include "globals.h"

void init_router()
{
	if (BirdConf)
	{
		int fd = open(BirdConf, O_RDWR|O_CREAT|O_TRUNC);
		if (!fd)
		{
			perror("Error opening Bird Config");
			exit(2);
		}
		char* conf = "table MeshVPN;\n";
		write(fd, conf, strlen(conf));
		close(fd);
	}

	if (Bird6Conf)
	{
		int fd = open(Bird6Conf, O_RDWR|O_CREAT|O_TRUNC);
		if (!fd)
		{
			perror("Error opening Bird6 Config");
			exit(2);
		}
		char* conf = "table MeshVPN;\n";
		write(fd, conf, strlen(conf));
		close(fd);
	}
}
void update_router()
{
	if (BirdConf)
	{
		int fd = open(BirdConf, O_RDWR|O_CREAT|O_TRUNC);
		if (!fd)
		{
			perror("Error opening Bird Config");
			exit(2);
		}
		char* conf = "table MeshVPN;\n";
		write(fd, conf, strlen(conf));
		struct Node* Node = FirstNode;
		while (Node)
		{
			if (inet_netof(Node->IPv4) != INADDR_ANY)
			{
				char NodeAddress[128];
				inet_ntop(AF_INET, &Node->IPv4, NodeAddress, sizeof(NodeAddress));
				char KeyStr[sizeof(node_key)*2+1];
				for (int x=0; x<sizeof(node_key); x++)
				{
					sprintf(KeyStr+(x*2), "%02x", Node->Key[x]);
				}
				char ConfEntry[1024];
				sprintf(ConfEntry, "protocol bgp VPN_%s {\n\ttable NotVPN;\n\timport all;\n\texport all;\n\tlocal as 65535;\n\tneighbor %s as 65535;\n\tdirect;\n};\n\n",
					KeyStr, NodeAddress);
				
				write(fd, ConfEntry, strlen(ConfEntry));
				write(fd, "\n", 1);
			}
			Node=Node->Next;
		}
		close(fd);
	}

	if (Bird6Conf)
	{
		int fd = open(Bird6Conf, O_RDWR|O_CREAT|O_TRUNC);
		if (!fd)
		{
			perror("Error opening Bird6 Config");
			exit(2);
		}
		char* conf = "table MeshVPN;\n";
		write(fd, conf, strlen(conf));
		struct Node* Node = FirstNode;
		while (Node)
		{
			if (memcmp(&Node->IPv6, &in6addr_any, sizeof(struct in6_addr)))
			{
				char NodeAddress[128];
				inet_ntop(AF_INET6, &Node->IPv6, NodeAddress, sizeof(NodeAddress));
				char KeyStr[sizeof(node_key)*2+1];
				for (int x=0; x<sizeof(node_key); x++)
				{
					sprintf(KeyStr+(x*2), "%02x", Node->Key[x]);
				}
				char ConfEntry[1024];
				sprintf(ConfEntry, "protocol bgp VPN_%s {\n\ttable NotVPN;\n\timport all;\n\texport all;\n\tlocal as 65535;\n\tneighbor %s as 65535;\n\tdirect;\n};\n\n",
					KeyStr, NodeAddress);
				
				write(fd, ConfEntry, strlen(ConfEntry));
				write(fd, "\n", 1);
			}
			Node=Node->Next;
		}
		close(fd);
	}
}