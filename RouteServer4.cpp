#include "Globals.h"

RouteServer4::RouteServer4(MeshVPN* app, Switch* Parent, struct in_addr Address, unsigned short Port)
{
	if (inet_netof(Address) != INADDR_ANY)
	{
		Socket = socket(AF_INET, SOCK_STREAM, 0);
		int optval = 1;
		setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
		struct sockaddr_in serveraddr;
		serveraddr.sin_family = AF_INET;
		memcpy(&serveraddr.sin_addr, &Address, 4);
		serveraddr.sin_port = htons(Port);
		while (bind(Socket, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
		{
			perror("ERROR binding TCP4");
			sleep(1);
		}
		while (listen(Socket, 5) < 0)
		{
			perror("ERROR on listen TCP4");
			sleep(1);
		}
		fcntl(Socket, F_SETFL, O_NONBLOCK);
		if (Debug)
		{
			char IP[128];
			inet_ntop(AF_INET, &Address, IP, sizeof(IP));
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
RouteServer4::~RouteServer4()
{
	close(Socket);
	if (Debug)
		printf("Closed TCP4 Port\n");
}
int RouteServer4::GetHandle()
{
	return Socket;
}
bool RouteServer4::ReadHandle()
{
	struct sockaddr_in clientaddr;
	unsigned int clientlen = sizeof(clientaddr);
	int ClientSock = accept(Socket, (struct sockaddr *) &clientaddr, &clientlen);
	if (ClientSock > -1)
	{
		for (RemoteNode* Node : Parent->RemoteNodes)
		{
			if (memcmp(Node->GetIPv4(), &clientaddr.sin_addr, 4) == 0)
			{
				fcntl(ClientSock, F_SETFL, O_NONBLOCK);
				RouteServer4Client* Client = new RouteServer4Client(ClientSock, this);
				for (struct Route4 &Route : Parent->NL4->Routes)
				{
					unsigned char Packet[1 + 4 + 1 + 4];
					Packet[0] = 1;
					memcpy(Packet + 1, &Route.Destination, 4);
					Packet[5] = Route.PrefixLength;
					*(unsigned int*)(Packet + 6) = htonl(Route.Metric);
					Client->Send(Packet, sizeof(Packet));
				}
				Clients.push_back(Client);
				app->AddSocket(Client);
				return true;
			}
		}
		if (Debug)
		{
			char IP[128];
			inet_ntop(AF_INET, &clientaddr.sin_addr, IP, sizeof(IP));
			printf("Rejected TCP client %s\n", IP);
		}
		close(ClientSock);
	}
	return false;
}
void RouteServer4::DeleteClient(RouteServer4Client* Client)
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
void RouteServer4::AdvertiseRoute(struct in_addr* Destination, int PrefixLength, int Metric)
{
	for (RouteServer4Client* Client : Clients)
	{
		unsigned char Packet[1 + 4 + 1 + 4];
		Packet[0] = 1;
		memcpy(Packet + 1, Destination, 4);
		Packet[5] = PrefixLength;
		*(unsigned int*)(Packet + 6) = htonl(Metric);
		Client->Send(Packet, sizeof(Packet));
	}
}
void RouteServer4::WithdrawRoute(struct in_addr* Destination, int PrefixLength, int Metric)
{
	for (RouteServer4Client* Client : Clients)
	{
		unsigned char Packet[1 + 4 + 1 + 4];
		Packet[0] = 0;
		memcpy(Packet + 1, Destination, 4);
		Packet[5] = PrefixLength;
		*(unsigned int*)(Packet + 6) = htonl(Metric);
		Client->Send(Packet, sizeof(Packet));
	}
}