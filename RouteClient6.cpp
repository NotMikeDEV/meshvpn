#include "Globals.h"

RouteClient6::RouteClient6(MeshVPN* app, RemoteNode* Parent, struct in6_addr* IPv6, unsigned short Port)
{
	this->app = app;
	this->Parent = Parent;
	memcpy(&this->IPv6, IPv6, 16);
	this->Port = Port;
	Reconnect();
	app->AddSocket(this);
}
void RouteClient6::Reconnect()
{
	Socket = socket(AF_INET6, SOCK_STREAM, 0);
	fcntl(Socket, F_SETFL, O_NONBLOCK);
	struct sockaddr_in6 remote = { 0 };
	remote.sin6_family = AF_INET6;
	memcpy(&remote.sin6_addr, &this->IPv6, sizeof(struct in6_addr));
	remote.sin6_port = htons(Port);

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
	inet_ntop(AF_INET6, &this->IPv6, IP, sizeof(IP));
	if (Debug)
		printf("CONNECT TCP6 TO %s %d\n", IP, Port);
}
RouteClient6::~RouteClient6()
{
	char IP[128];
	inet_ntop(AF_INET6, &this->IPv6, IP, sizeof(IP));
	if (Debug)
		printf("CLOSE TCP6 %s %d\n", IP, Port);
	close(Socket);
	app->DelSocket(this);
	for (int index = 0; index < Routes.size(); ++index)
	{
		Parent->ParentSwitch->NL6->DeleteRoute(Routes[index]);
	}
	Routes.erase(Routes.begin(), Routes.end());
}
int RouteClient6::GetHandle()
{
	return Socket;
}
bool RouteClient6::ReadHandle()
{
	int ret = recv(Socket, RecvBuffer + RecvOffset, sizeof(RecvBuffer) - RecvOffset, 0);
	if (ret > 0)
	{
		RecvOffset += ret;
		if (RecvOffset == sizeof(RecvBuffer))
		{
			RecvOffset = 0;
			Route6 Route;
			memcpy(&Route.Destination, RecvBuffer + 1, 16);
			Route.PrefixLength = RecvBuffer[17];
			memcpy(&Route.Gateway, &IPv6, 16);
			Route.OriginalMetric = htonl(*(unsigned int*)(RecvBuffer + 18));
			Route.Metric = htonl(*(unsigned int*)(RecvBuffer + 18)) + Parent->Latency;
			if (RecvBuffer[0] == 1)
			{
				if (Debug)
				{
					char IP[128];
					inet_ntop(AF_INET6, &Route.Destination, IP, sizeof(IP));
					char Gateway[128];
					inet_ntop(AF_INET6, &Route.Gateway, Gateway, sizeof(Gateway));
					printf("Got IPv6 Route %s/%u (%u) via %s\n", IP, Route.PrefixLength, Route.Metric, Gateway);
				}
				Parent->ParentSwitch->NL6->AddRoute(Route);
				Routes.push_back(Route);
			}
			if (RecvBuffer[0] == 0)
			{
				for (int index = 0; index < Routes.size(); ++index)
				{
					if (
						memcmp(&Routes[index].Destination, &Route.Destination, 16) == 0 &&
						Routes[index].PrefixLength == Route.PrefixLength &&
						memcmp(&Routes[index].Gateway, &Route.Gateway, 16) == 0 &&
						Routes[index].OriginalMetric == Route.OriginalMetric
						)
					{
						if (Debug)
						{
							char IP[128];
							inet_ntop(AF_INET6, &Routes[index].Destination, IP, sizeof(IP));
							char Gateway[128];
							inet_ntop(AF_INET6, &Routes[index].Gateway, Gateway, sizeof(Gateway));
							printf("Delete IPv6 Route %s/%u (%u) via %s\n", IP, Routes[index].PrefixLength, Routes[index].Metric, Gateway);
						}
						Parent->ParentSwitch->NL6->DeleteRoute(Routes[index]);
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
		inet_ntop(AF_INET6, &this->IPv6, IP, sizeof(IP));
		if (Debug)
			printf("CLOSE TCP6 %s %d\n", IP, Port);
		close(Socket);
		Reconnect();
	}
	return ret > -1;
}
