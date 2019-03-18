#include "Globals.h"
RemoteNode::RemoteNode(struct sockaddr_in6* Address, Switch* Switch, MeshVPN* app)
{
	memset(&IPv4, 0, sizeof(&IPv4));
	memset(&IPv6, 0, sizeof(&IPv6));
	this->app = app;
	Status = 0;
	Retries = 1;
	NextSend = 0;
	memset(&NodeKey, 0, sizeof(NodeKey));
	ParentSwitch = Switch;
	memset(&this->Address, 0, sizeof(this->Address));
	this->Address.sin6_family = AF_INET6;
	memcpy(&this->Address.sin6_addr, &Address->sin6_addr, sizeof(struct in6_addr));
	this->Address.sin6_port = Address->sin6_port;
	this->Latency = 1000;
}
RemoteNode::~RemoteNode()
{
	ParentSwitch->SendRemote(this, 0xEE, NULL, 0);
	if (RC4)
		delete RC4;
	if (RC6)
		delete RC6;
}
void RemoteNode::SetIPv4(struct in_addr* IP)
{
	if (Status && ParentSwitch->RoutingTable != -1 && memcmp(IP, &IPv4, 4))
	{
		if (RC4)
			delete RC4;
		memcpy(&IPv4, IP, 4);
		RC4 = new RouteClient4(app, this, &IPv4, htons(Address.sin6_port));
	}
	else
	{
		memcpy(&IPv4, IP, 4);
	}
}
void RemoteNode::SetIPv6(struct in6_addr* IP)
{
	if (Status && ParentSwitch->RoutingTable != -1 && memcmp(IP, &IPv6, 16))
	{
		if (RC6)
			delete RC6;
		memcpy(&IPv6, IP, 16);
		RC6 = new RouteClient6(app, this, &IPv6, htons(Address.sin6_port));
	}
	else
	{
		memcpy(&IPv6, IP, 16);
	}
}
struct in_addr* RemoteNode::GetIPv4()
{
	return &IPv4;
}
struct in6_addr* RemoteNode::GetIPv6()
{
	return &IPv6;
}

unsigned char RemoteNode::GetStatus()
{
	return Status;
}
void RemoteNode::SetStatus(unsigned char Status)
{
	if (Status)
	{
		if (ParentSwitch->RoutingTable != -1 && Status && !this->Status && inet_netof(IPv4) != INADDR_ANY)
		{
			RC4 = new RouteClient4(app, this, &IPv4, htons(Address.sin6_port));
		}
		if (ParentSwitch->RoutingTable != -1 && Status && !this->Status && memcmp(&IPv6, &in6addr_any, sizeof(struct in6_addr)))
		{
			RC6 = new RouteClient6(app, this, &IPv6, htons(Address.sin6_port));
		}
		Retries = 0;
	}
	else
	{
		MACs.erase(MACs.begin(), MACs.end());
		if (RC4)
		{
			delete RC4;
			RC4 = NULL;
		}
		if (RC6)
		{
			delete RC6;
			RC6 = NULL;
		}
	}
	this->Status = Status;
}
unsigned char RemoteNode::GetRetries()
{
	return Retries;
}
void RemoteNode::IncrementRetryCount()
{
	Retries++;
}
void RemoteNode::DoNextSend(time_t Time)
{
	if (Status == 0)
	{
		Retries++;
		NextSend = Time + 15;
		if (Debug)
		{
			char IPv6[128];
			inet_ntop(AF_INET6, &Address.sin6_addr, IPv6, sizeof(IPv6));
			printf("Send INIT to %s %u\n", IPv6, htons(Address.sin6_port));
		}
		ParentSwitch->SendRemote(this, 0x00, NULL, 0);
	}
	if (Status == 1)
	{
		NextSend = Time + 45;
		unsigned char Count = 0;
		unsigned char Packet[1024] = { 0 };
		for (RemoteNode* n : ParentSwitch->RemoteNodes)
		{
			if (n->Status == 0)
				continue;
			if (Count >= 50)
			{
				Packet[0] = Count;
				ParentSwitch->SendRemote(this, 0xF0, Packet, (18 * Count) + 1);
				Count = 0;
			}
			memcpy(Packet + 1 + (Count * 18), &n->Address.sin6_addr, 16);
			memcpy(Packet + 1 + (Count * 18) + 16, &n->Address.sin6_port, 2);
			Count++;
		}
		Packet[0] = Count;
		ParentSwitch->SendRemote(this, 0xF1, Packet, (18 * Count) + 1);
		gettimeofday(&PingSendTime, NULL);

		if (Debug)
		{
			char IPv6[128];
			inet_ntop(AF_INET6, &Address.sin6_addr, IPv6, sizeof(IPv6));
			printf("Send keep-alive to %s %u\n", IPv6, htons(Address.sin6_port));
		}
	}
}