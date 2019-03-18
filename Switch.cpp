#include "Globals.h"
Switch::Switch(MeshVPN* app)
{
	memset(&IPv4, 0, sizeof(&IPv4));
	memset(&IPv6, 0, sizeof(&IPv6));
	this->app = app;
}
Switch::~Switch()
{
	for (int index = 0; index < RemoteNodes.size(); ++index)
	{
		delete RemoteNodes[index];
		RemoteNodes.erase(RemoteNodes.begin() + index);
		index--;
	}
	if (NL4)
		delete NL4;
	if (RS4)
		delete RS4;
	if (NL6)
		delete NL6;
	if (RS6)
		delete RS6;
	delete Interface;
}
void Switch::Init(char* InterfaceName, unsigned short Port, int RoutingTable)
{
	int fd;
	if ((fd = open("/dev/urandom", O_RDONLY)) < 0)
	{
		perror("open /dev/urandom");
		exit(-1);
	}
	read(fd, NodeKey, sizeof(NodeKey));
	close(fd);
	if (Debug)
	{
		printf("Node Key: ");
		for (int i = 0; i < sizeof(NodeKey); i++) {
			printf("%02x", NodeKey[i]);
		}
		printf("\n");
	}

	struct sockaddr_in6 server_addr;
	Socket = socket(PF_INET6, SOCK_DGRAM, 0);
	if (Socket < 0) {
		perror("creating socket");
		exit(-1);
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin6_family = AF_INET6;
	server_addr.sin6_port = htons(Port);

	if (bind(Socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("bind failed");
		exit(-1);
	}
	socklen_t addrlen = sizeof(server_addr);
	getsockname(Socket, (struct sockaddr *)&server_addr, &addrlen);
	this->Port = htons(server_addr.sin6_port);
	fcntl(Socket, F_SETFL, O_NONBLOCK);
	if (Debug)
	{
		printf("Listening on UDP %u\n", this->Port);
	}
	this->RoutingTable = RoutingTable;
	Interface = new TunTap(InterfaceName, this);
	app->AddSocket(this);
	app->AddSocket(Interface);
	app->Init(this);
	if (inet_netof(this->IPv4) != INADDR_ANY)
	{
		Interface->SetIPv4(&this->IPv4, this->IPv4_PrefixLength);
		if (RoutingTable >= 0)
		{
			NL4 = new NetLink4(RoutingTable);
			app->AddSocket(NL4);
			RS4 = new RouteServer4(app, this, this->IPv4, (unsigned short)this->Port);
			NL4->AttachServer(RS4);
		}
	}
	if (memcmp(&this->IPv6, &in6addr_any, sizeof(struct in6_addr)))
	{
		Interface->SetIPv6(&this->IPv6, this->IPv6_PrefixLength);
		if (RoutingTable >= 0)
		{
			NL6 = new NetLink6(RoutingTable);
			app->AddSocket(NL6);
			RS6 = new RouteServer6(app, this, this->IPv6, (unsigned short)this->Port);
			NL6->AttachServer(RS6);
		}
	}
}
void Switch::SetIPv4(struct in_addr* IP, int PrefixLength)
{
	memcpy(&this->IPv4, IP, 4);
	IPv4_PrefixLength = PrefixLength;
}
void Switch::SetIPv6(struct in6_addr* IP, int PrefixLength)
{
	memcpy(&this->IPv6, IP, 16);
	IPv6_PrefixLength = PrefixLength;
}
struct in_addr* Switch::GetIPv4()
{
	return &IPv4;
}
struct in6_addr* Switch::GetIPv6()
{
	return &IPv6;
}
int Switch::GetHandle()
{
	return Socket;
}
bool Switch::ReadHandle()
{
	unsigned char buffer[4096];
	struct sockaddr_in6 addr;
	socklen_t addrlen = sizeof(addr);
	int nread = recvfrom(Socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addrlen);
	if (nread > 0)
	{
		unsigned char packet[sizeof(buffer)];
		int decrypted_len = Crypto::Decrypt(buffer, nread, packet);
		if (Debug > 3)
		{
			char IPv6[128];
			inet_ntop(AF_INET6, &addr.sin6_addr, IPv6, sizeof(IPv6));
			printf("Read %d bytes from %s %u\n", decrypted_len, IPv6, htons(addr.sin6_port));
		}
		unsigned char* FromKey = packet;
		unsigned char* ToKey = packet + sizeof(NodeKey);
		unsigned char Type = packet[sizeof(NodeKey) * 2];
		unsigned char* Payload = packet + 1 + (sizeof(NodeKey)*2);
		int Length = nread - 1 - (sizeof(NodeKey) * 2);
		if (Debug > 3)
		{
			printf("%u (%02X) packet from ", Length, Type);
			for (int i = 0; i < sizeof(NodeKey); i++) {
				printf("%02x", FromKey[i]);
			}
			printf(" to ");
			for (int i = 0; i < sizeof(NodeKey); i++) {
				printf("%02x", ToKey[i]);
			}
			printf("\n");
		}
		ParsePacket(Type, FromKey, ToKey, Payload, Length, &addr);
		return true;
	}
	return false;
}
void Switch::LocalEthernetPacket(unsigned char* Packet, int Length)
{
	for (RemoteNode* &n : RemoteNodes)
	{
		if (n->GetStatus())
		{
			for (struct MAC &E : n->MACs)
			{
				if (memcmp(E.a, Packet + 4, 6) == 0)
				{
					if (Debug > 2)
						printf("Unicasting packet from %02X%02X%02X%02X%02X%02X to %02X%02X%02X%02X%02X%02X\n", Packet[10], Packet[11], Packet[12], Packet[13], Packet[14], Packet[15], Packet[4], Packet[5], Packet[6], Packet[7], Packet[8], Packet[9]);
					SendRemote(n, 0xFF, Packet, Length);
					return;
				}
			}
		}
	}
	if (Debug > 2)
		printf("Broadcasting packet from %02X%02X%02X%02X%02X%02X to %02X%02X%02X%02X%02X%02X\n", Packet[10], Packet[11], Packet[12], Packet[13], Packet[14], Packet[15], Packet[4], Packet[5], Packet[6], Packet[7], Packet[8], Packet[9]);
	for (RemoteNode* n : RemoteNodes)
	{
		if (n->GetStatus())
			SendRemote(n, 0xFF, Packet, Length);
	}
}
void Switch::RemoteEthernetPacket(RemoteNode* Node, unsigned char* Packet, int Length)
{
	if (Debug > 2)
		printf("Inbound packet from %02X%02X%02X%02X%02X%02X to %02X%02X%02X%02X%02X%02X\n", Packet[10], Packet[11], Packet[12], Packet[13], Packet[14], Packet[15], Packet[4], Packet[5], Packet[6], Packet[7], Packet[8], Packet[9]);
	Interface->WritePacket(Packet, Length);
	for (struct MAC &E : Node->MACs)
	{
		if (memcmp(E.a, Packet + 10, 6) == 0)
		{
			return;
		}
	}
	struct MAC E;
	memcpy(E.a, Packet + 10, 6);
	Node->MACs.push_back(E);
	if (Debug > 1)
		printf("Added MAC to switching table\n");
}
RemoteNode* Switch::GetRemote(struct sockaddr_in6* Address)
{
	for (RemoteNode* n : RemoteNodes)
	{
		if (memcmp(&n->Address.sin6_addr, &Address->sin6_addr, sizeof(struct in6_addr)) == 0 && n->Address.sin6_port == Address->sin6_port)
		{
			return n;
		}
	}
	return NULL;
}
RemoteNode* Switch::GetRemote(unsigned char* NodeKey)
{
	for (RemoteNode* n : RemoteNodes)
	{
		if (memcmp(&n->NodeKey, NodeKey, sizeof(n->NodeKey)) == 0)
		{
			return n;
		}
	}
	return NULL;
}
void Switch::AddRemote(struct sockaddr_in6* Address, bool Static)
{
	for (RemoteNode* n : RemoteNodes)
	{
		if (memcmp(&n->Address.sin6_addr, &Address->sin6_addr, sizeof(struct in6_addr)) == 0 && n->Address.sin6_port == Address->sin6_port)
		{
			return;
		}
	}
	RemoteNode* Node = new RemoteNode(Address, this, app);
	Node->Static = Static;
	RemoteNodes.push_back(Node);
	if (Debug)
	{
		char IPv6[128];
		inet_ntop(AF_INET6, &Address->sin6_addr, IPv6, sizeof(IPv6));
		printf("Added remote node: %s %u\n", IPv6, htons(Address->sin6_port));
	}
}
void Switch::RemoveRemote(struct sockaddr_in6* Address)
{
	for (int index = 0; index < RemoteNodes.size(); ++index)
	{
		if (memcmp(&RemoteNodes[index]->Address.sin6_addr, &Address->sin6_addr, sizeof(struct in6_addr)) == 0 && RemoteNodes[index]->Address.sin6_port == Address->sin6_port)
		{
			delete RemoteNodes[index];
			RemoteNodes.erase(RemoteNodes.begin()+index);
			if (Debug)
			{
				char IPv6[128];
				inet_ntop(AF_INET6, &Address->sin6_addr, IPv6, sizeof(IPv6));
				printf("Forgot remote node: %s %u\n", IPv6, htons(Address->sin6_port));
			}
			return;
		}
	}
}
void Switch::SendRemote(RemoteNode* Node, unsigned char Type, unsigned char* Packet, int Length)
{
	unsigned char* full_packet = (unsigned char*) malloc(sizeof(Node->NodeKey) + sizeof(NodeKey) + 1 + Length);
	memcpy(full_packet, NodeKey, sizeof(NodeKey));
	memcpy(full_packet + sizeof(NodeKey), Node->NodeKey, sizeof(NodeKey));
	full_packet[sizeof(NodeKey)*2] = Type;
	if (Length)
		memcpy(full_packet + 1 + sizeof(NodeKey) + sizeof(Node->NodeKey), Packet, Length);
	unsigned char* ciphertext = (unsigned char*) malloc(sizeof(Node->NodeKey) + sizeof(NodeKey) + 1 + Length + 16);
	int cipherlen = Crypto::Encrypt(full_packet, sizeof(Node->NodeKey) + sizeof(NodeKey) + 1 + Length, ciphertext);
	sendto(Socket, ciphertext, cipherlen, 0, (struct sockaddr *)&Node->Address, sizeof(struct sockaddr_in6));
	free(full_packet);
	free(ciphertext);
}
void Switch::ParsePacket(unsigned char Type, unsigned char* FromKey, unsigned char* ToKey, unsigned char* Payload, int Length, struct sockaddr_in6* Address)
{
	if (memcmp(FromKey, NodeKey, sizeof(NodeKey)) == 0)
		return;
	switch (Type)
	{
		case 0x00:
		{
			AddRemote(Address, false);
			RemoteNode* Node = GetRemote(Address);
			if (Debug > 1)
			{
				char IPv6[128];
				inet_ntop(AF_INET6, &Address->sin6_addr, IPv6, sizeof(IPv6));
				printf("Recv INIT from %s %u\n", IPv6, htons(Address->sin6_port));
			}
			memcpy(&Node->NodeKey, FromKey, sizeof(Node->NodeKey));
			Node->NextSend = time(NULL) + 5;
			unsigned char IPs[4 + 16] = { 0 };
			if (RoutingTable != -1)
			{
				memcpy(IPs, GetIPv4(), 4);
				memcpy(IPs + 4, GetIPv6(), 16);
			}
			SendRemote(Node, 0x01, IPs, 4+16);
		}
		break;
		case 0x01:
		{
			if (memcmp(ToKey, NodeKey, sizeof(NodeKey)) == 0)
			{
				RemoteNode* Node = GetRemote(Address);
				if (Node)
				{
					Node->SetIPv4((struct in_addr*)Payload);
					Node->SetIPv6((struct in6_addr*)(Payload + 4));
					Node->SetStatus(1);
					Node->NextSend = time(NULL);
					Node->LastRecv = time(NULL);
					memcpy(&Node->NodeKey, FromKey, sizeof(Node->NodeKey));
					if (Debug > 1)
					{
						char IPv6[128];
						inet_ntop(AF_INET6, &Address->sin6_addr, IPv6, sizeof(IPv6));
						printf("Recv INIT RES from %s %u (", IPv6, htons(Address->sin6_port));
						for (int i = 0; i < sizeof(NodeKey); i++)
						{
							printf("%02x", FromKey[i]);
						}
						char IPv4[128];
						inet_ntop(AF_INET, Node->GetIPv4(), IPv4, sizeof(IPv4));
						inet_ntop(AF_INET6, Node->GetIPv6(), IPv6, sizeof(IPv6));
						printf(") %s %s\n", IPv4, IPv6);
					}
					unsigned char IPs[4 + 16] = { 0 };
					if (RoutingTable != -1)
					{
						memcpy(IPs, GetIPv4(), 4);
						memcpy(IPs + 4, GetIPv6(), 16);
					}
					SendRemote(Node, 0x02, IPs, 4 + 16);
				}
			}
		}
		break;
		case 0x02:
		{
			if (memcmp(ToKey, NodeKey, sizeof(NodeKey)) == 0)
			{
				RemoteNode* Node = GetRemote(FromKey);
				if (Node)
				{
					Node->SetIPv4((struct in_addr*)Payload);
					Node->SetIPv6((struct in6_addr*)(Payload + 4));
					Node->SetStatus(1);
					Node->NextSend = time(NULL);
					Node->LastRecv = time(NULL);
					if (Debug > 1)
					{
						char IPv6[128];
						inet_ntop(AF_INET6, &Address->sin6_addr, IPv6, sizeof(IPv6));
						printf("Recv INIT ACK from %s %u (", IPv6, htons(Address->sin6_port));
						for (int i = 0; i < sizeof(NodeKey); i++)
						{
							printf("%02x", FromKey[i]);
						}
						char IPv4[128];
						inet_ntop(AF_INET, Node->GetIPv4(), IPv4, sizeof(IPv4));
						inet_ntop(AF_INET6, Node->GetIPv6(), IPv6, sizeof(IPv6));
						printf(") %s %s\n", IPv4, IPv6);
					}
				}
			}
		}
		break;
		case 0xF1:
		{
			if (memcmp(ToKey, NodeKey, sizeof(NodeKey)) == 0)
			{
				RemoteNode* Node = GetRemote(FromKey);
				if (Node)
				{
					SendRemote(Node, 0xF2, NULL, 0);
				}
			}
		}
		case 0xF0:
		{
			if (memcmp(ToKey, NodeKey, sizeof(NodeKey)) == 0)
			{
				RemoteNode* Node = GetRemote(FromKey);
				if (Node)
				{
					Node->LastRecv = time(NULL);
					if (Debug)
					{
						char IPv6[128];
						inet_ntop(AF_INET6, &Address->sin6_addr, IPv6, sizeof(IPv6));
						printf("Recv KeepAlive from %s %u (", IPv6, htons(Address->sin6_port));
						for (int i = 0; i < sizeof(NodeKey); i++)
						{
							printf("%02x", FromKey[i]);
						}
						printf(") with %u nodes\n", Payload[0]);
					}
					sockaddr_in6 a;
					int Count = Payload[0];
					for (int x = 0; x < Count; x++)
					{
						memcpy(&a.sin6_addr, Payload + 1 + (x * 18), 16);
						memcpy(&a.sin6_port, Payload + 1 + (x * 18)+16, 2);
						AddRemote(&a, false);
						if (Debug > 1)
						{
							char IPv6[128];
							inet_ntop(AF_INET6, &a.sin6_addr, IPv6, sizeof(IPv6));
							printf("Learned Node: %s %u\n", IPv6, htons(a.sin6_port));
						}
					}
				}
			}
		}
		break;
		case 0xF2:
		{
			if (memcmp(ToKey, NodeKey, sizeof(NodeKey)) == 0)
			{
				RemoteNode* Node = GetRemote(FromKey);
				if (Node)
				{
					Node->LastRecv = time(NULL);
					struct timeval ReplyTime = { 0 };
					gettimeofday(&ReplyTime, NULL);
					struct timeval diff = { 0 };
					diff.tv_sec = ReplyTime.tv_sec - Node->PingSendTime.tv_sec;
					diff.tv_usec = ReplyTime.tv_usec - Node->PingSendTime.tv_usec;
					if (diff.tv_usec < 1)
					{
						diff.tv_usec += 1000000;
						diff.tv_sec -= 1;
					}
					Node->Latency = diff.tv_sec * 1000 + diff.tv_usec / 1000;
					if (Debug > 1)
					{
						char IPv6[128];
						inet_ntop(AF_INET6, &Address->sin6_addr, IPv6, sizeof(IPv6));
						printf("Recv Ping reply from %s %u (", IPv6, htons(Address->sin6_port));
						for (int i = 0; i < sizeof(NodeKey); i++)
						{
							printf("%02x", FromKey[i]);
						}
						printf(") %ums\n", Node->Latency);
					}
				}
			}
		}
		break;
		case 0xFF:
		{
			if (memcmp(ToKey, NodeKey, sizeof(NodeKey)) == 0)
			{
				RemoteNode* Node = GetRemote(FromKey);
				if (Node)
				{
					Node->LastRecv = time(NULL);
					if (Debug > 1)
					{
						char IPv6[128];
						inet_ntop(AF_INET6, &Address->sin6_addr, IPv6, sizeof(IPv6));
						printf("Recv Packet from %s %u (", IPv6, htons(Address->sin6_port));
						for (int i = 0; i < sizeof(NodeKey); i++)
						{
							printf("%02x", FromKey[i]);
						}
						printf(")\n");
					}
					RemoteEthernetPacket(Node, Payload, Length);
				}
			}
		}
		break;
		case 0xEE:
		{
			if (memcmp(ToKey, NodeKey, sizeof(NodeKey)) == 0)
			{
				RemoteNode* Node = GetRemote(FromKey);
				if (Node)
				{
					Node->SetStatus(0);
				}
			}
		}
		break;
	}
}
void Switch::RunInterval()
{
	time_t Time = time(NULL);
	int OnlineCount = 0;
	for (RemoteNode* n : RemoteNodes)
	{
		if (n->GetStatus() && Time >= n->LastRecv + 60)
		{
			n->SetStatus(0);
			n->NextSend = Time;
		}
		else if (Time >= n->NextSend)
		{
			n->DoNextSend(Time);
		}
		if (n->GetStatus())
			OnlineCount++;
	}
	for (RemoteNode* n : RemoteNodes)
	{
		if (OnlineCount && n->GetStatus() == 0 && n->GetRetries() > 3 && !n->Static)
		{
			RemoveRemote(&n->Address);
		}
	}
}