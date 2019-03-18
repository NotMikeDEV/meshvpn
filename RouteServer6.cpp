#include "Globals.h"

RouteServer6::RouteServer6(MeshVPN* app, Switch* Parent, struct in6_addr Address, unsigned short Port)
{
	if (memcmp(&Address, &in6addr_any, sizeof(struct in6_addr)))
	{
		Socket = socket(AF_INET6, SOCK_STREAM, 0);
		int optval = 1;
		setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
		struct sockaddr_in6 serveraddr;
		serveraddr.sin6_family = AF_INET6;
		memcpy(&serveraddr.sin6_addr, &Address, 16);
		serveraddr.sin6_port = htons(Port);
		while (bind(Socket, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
		{
			perror("ERROR binding TCP6");
			sleep(1);
		}
		while (listen(Socket, 5) < 0)
		{
			perror("ERROR on listen TCP6");
			sleep(1);
		}
		fcntl(Socket, F_SETFL, O_NONBLOCK);
		if (Debug)
		{
			char IP[128];
			inet_ntop(AF_INET6, &Address, IP, sizeof(IP));
			printf("Listening on TCP %s %u\n", IP, Port);
		}
		this->app = app;
		app->AddSocket(this);
		this->Parent = Parent;
	}
	else
	{
		Socket = -1;
	}
}
RouteServer6::~RouteServer6()
{
	close(Socket);
	if (Debug)
		printf("Closed TCP6 Port\n");
}
int RouteServer6::GetHandle()
{
	return Socket;
}
bool RouteServer6::ReadHandle()
{
	struct sockaddr_in6 clientaddr;
	unsigned int clientlen = sizeof(clientaddr);
	int ClientSock = accept(Socket, (struct sockaddr *) &clientaddr, &clientlen);
	if (ClientSock > -1)
	{
		for (RemoteNode* Node : Parent->RemoteNodes)
		{
			if (memcmp(Node->GetIPv6(), &clientaddr.sin6_addr, 16) == 0)
			{
				fcntl(ClientSock, F_SETFL, O_NONBLOCK);
				RouteServer6Client* Client = new RouteServer6Client(ClientSock, this);
				for (struct Route6 &Route : Parent->NL6->Routes)
				{
					unsigned char Packet[1 + 16 + 1 + 4];
					Packet[0] = 1;
					memcpy(Packet + 1, &Route.Destination, 16);
					Packet[17] = Route.PrefixLength;
					*(unsigned int*)(Packet + 18) = htonl(Route.Metric);
					Client->Send(Packet, sizeof(Packet));
				}
				Clients.push_back(Client);
				app->AddSocket(Client);
				return true;
			}
		}
		close(ClientSock);
	}
	return false;
}
void RouteServer6::DeleteClient(RouteServer6Client* Client)
{
	for (int index = 0; index < Clients.size(); ++index)
	{
		if (Clients[index] == Client)
		{
			app->DelSocket(Client);
			Clients.erase(Clients.begin() + index);
			delete Client;
			return;
		}
	}
}
void RouteServer6::AdvertiseRoute(struct in6_addr* Destination, int PrefixLength, int Metric)
{
	for (RouteServer6Client* Client : Clients)
	{
		unsigned char Packet[1 + 16 + 1 + 4];
		Packet[0] = 1;
		memcpy(Packet + 1, Destination, 16);
		Packet[17] = PrefixLength;
		*(unsigned int*)(Packet + 18) = htonl(Metric);
		Client->Send(Packet, sizeof(Packet));
	}
}
void RouteServer6::WithdrawRoute(struct in6_addr* Destination, int PrefixLength, int Metric)
{
	for (RouteServer6Client* Client : Clients)
	{
		unsigned char Packet[1 + 16 + 1 + 4];
		Packet[0] = 0;
		memcpy(Packet + 1, Destination, 16);
		Packet[17] = PrefixLength;
		*(unsigned int*)(Packet + 18) = htonl(Metric);
		Client->Send(Packet, sizeof(Packet));
	}
}