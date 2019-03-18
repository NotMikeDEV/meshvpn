#include "Globals.h"

RouteClient4::RouteClient4(MeshVPN* app, RemoteNode* Parent, struct in_addr* IPv4, unsigned short Port)
{
	this->app = app;
	this->Parent = Parent;
	memcpy(&this->IPv4, IPv4, 4);
	this->Port = Port;
	Reconnect();
	app->AddSocket(this);
}
void RouteClient4::Reconnect()
{
	Socket = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(Socket, F_SETFL, O_NONBLOCK);
	struct sockaddr_in remote = { 0 };
	remote.sin_family = AF_INET;
	memcpy(&remote.sin_addr, &this->IPv4, sizeof(struct in_addr));
	remote.sin_port = htons(Port);

	connect(Socket, (struct sockaddr*)&remote, sizeof(remote));
	int KeepAlive = 1;
	setsockopt(Socket, SOL_SOCKET, SO_KEEPALIVE, &KeepAlive, sizeof(KeepAlive));
	int keepcnt = 3;
	setsockopt(Socket, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int));
	int keepidle = 15;
	setsockopt(Socket, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int));
	int keepintvl = 120;
	setsockopt(Socket, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(int));

	char IP[128];
	inet_ntop(AF_INET, &this->IPv4, IP, sizeof(IP));
	if (Debug)
		printf("CONNECT TCP4 TO %s %d\n", IP, Port);
}
RouteClient4::~RouteClient4()
{
	char IP[128];
	inet_ntop(AF_INET, &this->IPv4, IP, sizeof(IP));
	if (Debug)
		printf("CLOSE TCP4 %s %d\n", IP, Port);
	close(Socket);
	app->DelSocket(this);
	for (int index = 0; index < Routes.size(); ++index)
	{
		Parent->ParentSwitch->NL4->DeleteRoute(Routes[index]);
	}
	Routes.erase(Routes.begin(), Routes.end());
}
int RouteClient4::GetHandle()
{
	return Socket;
}
bool RouteClient4::ReadHandle()
{
	int ret = recv(Socket, RecvBuffer + RecvOffset, sizeof(RecvBuffer) - RecvOffset, 0);
	if (ret > 0)
	{
		RecvOffset += ret;
		if (RecvOffset == sizeof(RecvBuffer))
		{
			RecvOffset = 0;
			Route4 Route;
			memcpy(&Route.Destination, RecvBuffer + 1, 4);
			Route.PrefixLength = RecvBuffer[5];
			memcpy(&Route.Gateway, &IPv4, 4);
			Route.OriginalMetric = htonl(*(unsigned int*)(RecvBuffer + 6));
			Route.Metric = htonl(*(unsigned int*)(RecvBuffer + 6)) + Parent->Latency;
			if (RecvBuffer[0] == 1)
			{
				if (Debug)
				{
					char IP[128];
					inet_ntop(AF_INET, &Route.Destination, IP, sizeof(IP));
					char Gateway[128];
					inet_ntop(AF_INET, &Route.Gateway, Gateway, sizeof(Gateway));
					printf("Got IPv4 Route %s/%u (%u) via %s\n", IP, Route.PrefixLength, Route.Metric, Gateway);
				}
				Parent->ParentSwitch->NL4->AddRoute(Route);
				Routes.push_back(Route);
			}
			if (RecvBuffer[0] == 0)
			{
				for (int index = 0; index < Routes.size(); ++index)
				{
					if (
						memcmp(&Routes[index].Destination, &Route.Destination, 4) == 0 && 
						Routes[index].PrefixLength == Route.PrefixLength && 
						memcmp(&Routes[index].Gateway, &Route.Gateway, 4) == 0 && 
						Routes[index].OriginalMetric == Route.OriginalMetric
						)
					{
						if (Debug)
						{
							char IP[128];
							inet_ntop(AF_INET, &Routes[index].Destination, IP, sizeof(IP));
							char Gateway[128];
							inet_ntop(AF_INET, &Routes[index].Gateway, Gateway, sizeof(Gateway));
							printf("Delete IPv4 Route %s/%u (%u) via %s\n", IP, Routes[index].PrefixLength, Routes[index].Metric, Gateway);
						}
						Parent->ParentSwitch->NL4->DeleteRoute(Routes[index]);
						Routes.erase(Routes.begin() + index);
						return true;
					}
				}
			}
		}
	}
	if (ret == 0)
	{
		char IP[128];
		inet_ntop(AF_INET, &this->IPv4, IP, sizeof(IP));
		if (Debug)
			printf("CLOSE TCP4 %s %d\n", IP, Port);
		close(Socket);
		Reconnect();
	}
	return ret > -1;
}
