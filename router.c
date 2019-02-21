#include "globals.h"

void router_flush_data(struct RouterClient* Client)
{
	int tosend=Client->SendBufferLength;
	if (tosend > 4096)
		tosend=4096;
	int ret=send(Client->Socket, Client->SendBuffer, tosend, 0);
	while (ret == tosend)
	{
		Client->SendBufferLength -= ret;
		memmove(Client->SendBuffer, Client->SendBuffer+ret, Client->SendBufferLength);
		tosend=Client->SendBufferLength;
		if (tosend == 0)
			return;
		if (tosend > 4096)
			tosend=4096;
		ret=send(Client->Socket, Client->SendBuffer, tosend, 0);
	}
	if (ret > 0)
	{
		Client->SendBufferLength -= ret;
		memmove(Client->SendBuffer, Client->SendBuffer+ret, Client->SendBufferLength);
	}
}
void router_send_data(struct RouterClient* Client, unsigned char* buffer, int len)
{
	Client->SendBuffer = realloc(Client->SendBuffer, Client->SendBufferLength+len);
	memcpy(Client->SendBuffer+Client->SendBufferLength, buffer, len);
	Client->SendBufferLength+=len;
}
void router_add_ipv4client(int ClientSock)
{
	struct RouterClient* NewClient = malloc(sizeof(struct RouterClient));
	memset(NewClient, 0, sizeof(struct RouterClient));
	NewClient->Socket = ClientSock;
	NewClient->Next = RouterClients4;
	if (RouterClients4)
		RouterClients4->Previous = NewClient;
	RouterClients4 = NewClient;

	struct RouteEntry4* CurrentRoute = FirstRoute4;
	while (CurrentRoute)
	{
		struct route4 Route;
		Route.type=1;
		memcpy(&Route.dest, &CurrentRoute->Route.dest, sizeof(struct in_addr));
		Route.pl = CurrentRoute->Route.pl;
		Route.metric = CurrentRoute->Route.metric;
		router_send_data(NewClient, (unsigned char*)&Route, sizeof(Route));
		CurrentRoute = CurrentRoute->Next;
	}
}
void router_add_ipv6client(int ClientSock)
{
	struct RouterClient* NewClient = malloc(sizeof(struct RouterClient));
	memset(NewClient, 0, sizeof(struct RouterClient));
	NewClient->Socket = ClientSock;
	NewClient->Next = RouterClients6;
	if (RouterClients6)
		RouterClients6->Previous = NewClient;
	RouterClients6 = NewClient;

	struct RouteEntry6* CurrentRoute = FirstRoute6;
	while (CurrentRoute)
	{
		struct route6 Route;
		Route.type=1;
		memcpy(&Route.dest, &CurrentRoute->Route.dest, sizeof(struct in6_addr));
		Route.pl = CurrentRoute->Route.pl;
		Route.metric = CurrentRoute->Route.metric;
		router_send_data(NewClient, (unsigned char*)&Route, sizeof(Route));
		CurrentRoute = CurrentRoute->Next;
	}
}
void router_poll_v6clients()
{
	struct RouterClient* Client = RouterClients6;
	fd_set read_fds;
	FD_ZERO(&read_fds);
	int MAX_FD=0;
	while (Client)
	{
		FD_SET(Client->Socket, &read_fds);
		MAX_FD = MAX(MAX_FD, Client->Socket);
		Client=Client->Next;
	}
	struct timeval timeout={0};
	if (select(MAX_FD+1, &read_fds, NULL, NULL, &timeout))
	{
		Client = RouterClients6;
		while (Client)
		{
			if (FD_ISSET(Client->Socket, &read_fds))
			{
				char buffer[100];
				int ret = recv(Client->Socket, buffer, sizeof(buffer), 0);
				if (ret == 0)
				{
					if (Client->Next)
						Client->Next->Previous = Client->Previous;
					if (Client->Previous)
						Client->Previous->Next = Client->Next;
					if (Client == RouterClients6)
						RouterClients6 = Client->Next;
					if (Client->SendBuffer)
						free(Client->SendBuffer);
					free(Client);
				}
			}
			Client=Client->Next;
		}
	}
}
void router_poll_v4clients()
{
	struct RouterClient* Client = RouterClients4;
	fd_set read_fds;
	FD_ZERO(&read_fds);
	int MAX_FD=0;
	while (Client)
	{
		FD_SET(Client->Socket, &read_fds);
		MAX_FD = MAX(MAX_FD, Client->Socket);
		Client=Client->Next;
	}
	struct timeval timeout={0};
	if (select(MAX_FD+1, &read_fds, NULL, NULL, &timeout))
	{
		Client = RouterClients4;
		while (Client)
		{
			if (FD_ISSET(Client->Socket, &read_fds))
			{
				char buffer[100];
				int ret = recv(Client->Socket, buffer, sizeof(buffer), 0);
				if (ret == 0)
				{
					if (Client->Next)
						Client->Next->Previous = Client->Previous;
					if (Client->Previous)
						Client->Previous->Next = Client->Next;
					if (Client == RouterClients4)
						RouterClients4 = Client->Next;
					if (Client->SendBuffer)
						free(Client->SendBuffer);
					free(Client);
				}
			}
			Client=Client->Next;
		}
	}
}
struct RouteEntry4* route_add4(struct route4* ROUTE)
{
	if (!FirstRoute4)
	{
		FirstRoute4 = malloc(sizeof(struct RouteEntry4));
		memset(FirstRoute4, 0, sizeof(struct RouteEntry4));
		memcpy(&FirstRoute4->Route, ROUTE, sizeof(struct route4));
		if (Debug > 1)
		{
			char Dest[32]={0};
			inet_ntop(AF_INET, &ROUTE->dest, Dest, 32);
			printf("ADD %s/%d metric %d\n", Dest, ROUTE->pl, ROUTE->metric);
		}
		struct route4 Route;
		Route.type=1;
		memcpy(&Route.dest, &ROUTE->dest, sizeof(struct in_addr));
		Route.pl = ROUTE->pl;
		Route.metric = ROUTE->metric;
		
		struct RouterClient* Client = RouterClients4;
		while (Client)
		{
			router_send_data(Client, (unsigned char*)&Route, sizeof(Route));
			Client=Client->Next;
		}
		return FirstRoute4;
	}
	struct RouteEntry4* CurrentRoute = FirstRoute4;
	while(CurrentRoute)
	{
		if (memcmp(&ROUTE->dest, &CurrentRoute->Route.dest, 4) == 0 && CurrentRoute->Route.pl==ROUTE->pl && CurrentRoute->Route.metric == ROUTE->metric)
		{
			return CurrentRoute;
		}
		if (!CurrentRoute->Next)
		{
			CurrentRoute->Next = malloc(sizeof(struct RouteEntry4));
			memset(CurrentRoute->Next, 0, sizeof(struct RouteEntry4));
			CurrentRoute->Next->Previous = CurrentRoute;
			memcpy(&CurrentRoute->Next->Route, ROUTE, sizeof(struct route4));
			if (Debug > 1)
			{
				char Dest[32]={0};
				inet_ntop(AF_INET, &ROUTE->dest, Dest, 32);
				printf("ADD %s/%d metric %d\n", Dest, ROUTE->pl, ROUTE->metric);
			}
			struct route4 Route;
			Route.type=1;
			memcpy(&Route.dest, &ROUTE->dest, sizeof(struct in_addr));
			Route.pl = ROUTE->pl;
			Route.metric = ROUTE->metric;
			struct RouterClient* Client = RouterClients4;
			while (Client)
			{
				router_send_data(Client, (unsigned char*)&Route, sizeof(Route));
				Client=Client->Next;
			}
			return CurrentRoute->Next;
		}
		CurrentRoute=CurrentRoute->Next;
	}
}
void route_delete4(struct route4* ROUTE)
{
	struct RouteEntry4* CurrentRoute = FirstRoute4;
	while(CurrentRoute)
	{
		if (memcmp(&ROUTE->dest, &CurrentRoute->Route.dest, 4) == 0 && CurrentRoute->Route.pl==ROUTE->pl && CurrentRoute->Route.metric == ROUTE->metric)
		{
			if (CurrentRoute->Next)
				CurrentRoute->Next->Previous = CurrentRoute->Previous;
			if (CurrentRoute->Previous)
				CurrentRoute->Previous->Next = CurrentRoute->Next;
			if (CurrentRoute == FirstRoute4)
				FirstRoute4 = CurrentRoute->Next;
			free(CurrentRoute);
			if (Debug>1)
			{
				char Dest[32]={0};
				inet_ntop(AF_INET, &ROUTE->dest, Dest, 32);
				printf("DEL %s/%d metric %d\n", Dest, ROUTE->pl, ROUTE->metric);
			}
			struct route4 Route;
			Route.type=0;
			memcpy(&Route.dest, &ROUTE->dest, sizeof(struct in_addr));
			Route.pl = ROUTE->pl;
			Route.metric = ROUTE->metric;
			struct RouterClient* Client = RouterClients4;
			while (Client)
			{
				router_send_data(Client, (unsigned char*)&Route, sizeof(Route));
				Client=Client->Next;
			}
			return;
		}
		CurrentRoute=CurrentRoute->Next;
	}
}
struct RouteEntry6* route_add6(struct route6* ROUTE)
{
	if (!FirstRoute6)
	{
		FirstRoute6 = malloc(sizeof(struct RouteEntry6));
		memset(FirstRoute6, 0, sizeof(struct RouteEntry6));
		memcpy(&FirstRoute6->Route, ROUTE, sizeof(struct route6));
		if (Debug > 1)
		{
			char Dest[72]={0};
			inet_ntop(AF_INET6, &ROUTE->dest, Dest, 72);
			printf("ADD %s/%d metric %d\n", Dest, ROUTE->pl, ROUTE->metric);
		}
		struct route6 Route;
		Route.type=1;
		memcpy(&Route.dest, &ROUTE->dest, sizeof(struct in6_addr));
		Route.pl = ROUTE->pl;
		Route.metric = ROUTE->metric;
		struct RouterClient* Client = RouterClients6;
		while (Client)
		{
			router_send_data(Client, (unsigned char*)&Route, sizeof(Route));
			Client=Client->Next;
		}
		return FirstRoute6;
	}
	struct RouteEntry6* CurrentRoute = FirstRoute6;
	while(CurrentRoute)
	{
		if (memcmp(&ROUTE->dest, &CurrentRoute->Route.dest, 16) == 0 && CurrentRoute->Route.pl==ROUTE->pl && CurrentRoute->Route.metric == ROUTE->metric)
		{
			return CurrentRoute;
		}
		if (!CurrentRoute->Next)
		{
			CurrentRoute->Next = malloc(sizeof(struct RouteEntry6));
			memset(CurrentRoute->Next, 0, sizeof(struct RouteEntry6));
			CurrentRoute->Next->Previous = CurrentRoute;
			memcpy(&CurrentRoute->Next->Route, ROUTE, sizeof(struct route6));
			if (Debug > 1)
			{
				char Dest[72]={0};
				inet_ntop(AF_INET6, &ROUTE->dest, Dest, 72);
				printf("ADD %s/%d metric %d\n", Dest, ROUTE->pl, ROUTE->metric);
			}
			struct route6 Route;
			Route.type=1;
			memcpy(&Route.dest, &ROUTE->dest, sizeof(struct in6_addr));
			Route.pl = ROUTE->pl;
			Route.metric = ROUTE->metric;
			struct RouterClient* Client = RouterClients6;
			while (Client)
			{
				router_send_data(Client, (unsigned char*)&Route, sizeof(Route));
				Client=Client->Next;
			}
			return CurrentRoute->Next;
		}
		CurrentRoute=CurrentRoute->Next;
	}
}
void route_delete6(struct route6* ROUTE)
{
	struct RouteEntry6* CurrentRoute = FirstRoute6;
	while(CurrentRoute)
	{
		if (memcmp(&ROUTE->dest, &CurrentRoute->Route.dest, 16) == 0 && CurrentRoute->Route.pl==ROUTE->pl && CurrentRoute->Route.metric == ROUTE->metric)
		{
			if (CurrentRoute->Next)
				CurrentRoute->Next->Previous = CurrentRoute->Previous;
			if (CurrentRoute->Previous)
				CurrentRoute->Previous->Next = CurrentRoute->Next;
			if (CurrentRoute == FirstRoute6)
				FirstRoute6 = CurrentRoute->Next;
			if (Debug > 1)
			{
				char Dest[72]={0};
				inet_ntop(AF_INET6, &CurrentRoute->Route.dest, Dest, 72);
				printf("DEL ROUTE %s/%d metric %d\n", Dest, CurrentRoute->Route.pl, CurrentRoute->Route.metric);
			}
			struct route6 Route;
			Route.type=0;
			memcpy(&Route.dest, &ROUTE->dest, sizeof(struct in6_addr));
			Route.pl = ROUTE->pl;
			Route.metric = ROUTE->metric;
			struct RouterClient* Client = RouterClients6;
			while (Client)
			{
				router_send_data(Client, (unsigned char*)&Route, sizeof(Route));
				Client=Client->Next;
			}
			free(CurrentRoute);
			return;
		}
		CurrentRoute=CurrentRoute->Next;
	}
}
struct route4 parse_route4(struct nlmsghdr *nlh)
{
	struct route4 ROUTE={0};
	struct  rtmsg *route_entry;  /* This struct represent a route entry \
					in the routing table */
	struct  rtattr *route_attribute; /* This struct contain route \
						attributes (route type) */
	int     route_attribute_len = 0;
	route_entry = (struct rtmsg *) NLMSG_DATA(nlh);

	if (route_entry->rtm_family != AF_INET)
		return ROUTE;
	if (route_entry->rtm_table != ROUTE_TABLE)
		return ROUTE;
	if (route_entry->rtm_protocol == ROUTE_PROTO)
		return ROUTE;
	
	ROUTE.pl = route_entry->rtm_dst_len;
	ROUTE.type = nlh->nlmsg_type;
	route_attribute = (struct rtattr *) RTM_RTA(route_entry);
	/* Get the route atttibutes len */
	route_attribute_len = RTM_PAYLOAD(nlh);
	int metric = 0;
	/* Loop through all attributes */
	while(RTA_OK(route_attribute, route_attribute_len))
	{
		/* Get the destination address */
		if (route_attribute->rta_type == RTA_DST)
		{
			memcpy(&ROUTE.dest, RTA_DATA(route_attribute), sizeof(ROUTE.dest));
		}
		if (route_attribute->rta_type == RTA_PRIORITY)
		{
			ROUTE.metric = *(int*)RTA_DATA(route_attribute);
		}
		route_attribute = RTA_NEXT(route_attribute, route_attribute_len);
	}
	return ROUTE;
}

struct route6 parse_route6(struct nlmsghdr *nlh)
{
	struct route6 ROUTE={0};
	struct  rtmsg *route_entry;  /* This struct represent a route entry \
					in the routing table */
	struct  rtattr *route_attribute; /* This struct contain route \
						attributes (route type) */
	int     route_attribute_len = 0;
	route_entry = (struct rtmsg *) NLMSG_DATA(nlh);

	if (route_entry->rtm_family != AF_INET6)
		return ROUTE;
	if (route_entry->rtm_table != ROUTE_TABLE)
		return ROUTE;
	if (route_entry->rtm_protocol == ROUTE_PROTO)
		return ROUTE;
	
	ROUTE.pl = route_entry->rtm_dst_len;
	ROUTE.type = nlh->nlmsg_type;
	route_attribute = (struct rtattr *) RTM_RTA(route_entry);
	/* Get the route atttibutes len */
	route_attribute_len = RTM_PAYLOAD(nlh);
	int metric = 0;
	/* Loop through all attributes */
	while(RTA_OK(route_attribute, route_attribute_len))
	{
		/* Get the destination address */
		if (route_attribute->rta_type == RTA_DST)
		{
			memcpy(&ROUTE.dest, RTA_DATA(route_attribute), sizeof(ROUTE.dest));
		}
		if (route_attribute->rta_type == RTA_PRIORITY)
		{
			ROUTE.metric = *(int*)RTA_DATA(route_attribute);
		}
		route_attribute = RTA_NEXT(route_attribute, route_attribute_len);
	}
	return ROUTE;
}

void parse_netlink(struct nlmsghdr *nh)
{
	struct route4 ROUTE4 = parse_route4(nh);
	if (ROUTE4.type == RTM_NEWROUTE)
	{
		route_add4(&ROUTE4);
	}
	if (ROUTE4.type == RTM_DELROUTE)
	{
		route_delete4(&ROUTE4);
	}
	struct route6 ROUTE6 = parse_route6(nh);
	if (ROUTE6.type == RTM_NEWROUTE)
	{
		route_add6(&ROUTE6);
	}
	if (ROUTE6.type == RTM_DELROUTE)
	{
		route_delete6(&ROUTE6);
	}
}
void init_router()
{
	RouterClients4 = NULL;
	RouterClients6 = NULL;
	if (inet_netof(node_IPv4) != INADDR_ANY)
	{
		ROUTER_SOCK4 = socket(AF_INET, SOCK_STREAM, 0);
		int optval = 1;
		setsockopt(ROUTER_SOCK4, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
		struct sockaddr_in serveraddr;
		serveraddr.sin_family = AF_INET;
		memcpy(&serveraddr.sin_addr, &node_IPv4, 4);
		serveraddr.sin_port = htons(Port);
		while (bind(ROUTER_SOCK4, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
		{
			perror("ERROR binding TCP4");
			sleep(1);
		}
		while (listen(ROUTER_SOCK4, 5) < 0)
		{
			perror("ERROR on listen");
			sleep(1);
		}
		fcntl(ROUTER_SOCK4, F_SETFL, O_NONBLOCK);
	}
	if (memcmp(&node_IPv6, &in6addr_any, sizeof(struct in6_addr)))
	{
		ROUTER_SOCK6 = socket(AF_INET6, SOCK_STREAM, 0);
		int optval = 1;
		setsockopt(ROUTER_SOCK6, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
		struct sockaddr_in6 serveraddr;
		serveraddr.sin6_family = AF_INET6;
		memcpy(&serveraddr.sin6_addr, &node_IPv6, 16);
		serveraddr.sin6_port = htons(Port);
		while (bind(ROUTER_SOCK6, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
		{
			perror("ERROR binding TCP6");
			sleep(1);
		}
		while (listen(ROUTER_SOCK6, 5) < 0)
		{
			perror("ERROR on listen");
			sleep(1);
		}
		fcntl(ROUTER_SOCK6, F_SETFL, O_NONBLOCK);
	}
	NLSOCK = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
	struct sockaddr_nl nladdr={0};
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = getpid();
	nladdr.nl_groups = RTMGRP_IPV4_ROUTE|RTMGRP_IPV6_ROUTE;
	bind(NLSOCK, (struct sockaddr*)&nladdr, sizeof(nladdr));
	struct nl_req_t request={0};
	request.hdr.nlmsg_len = sizeof(request);
	request.hdr.nlmsg_type = RTM_GETROUTE;
	request.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	request.hdr.nlmsg_seq = 1;
	request.hdr.nlmsg_pid = getpid();
	request.msg.rtm_family = AF_INET6;
	request.msg.rtm_table = ROUTE_TABLE;
	struct iovec io={0};
	io.iov_base = &request;
	io.iov_len = request.hdr.nlmsg_len;
	struct msghdr rtnl_msg={0};
	rtnl_msg.msg_iov = &io;
	rtnl_msg.msg_iovlen = 1;
	sendmsg(NLSOCK, (struct msghdr *) &rtnl_msg, 0);
	char Loading6=1;
	if (Debug)
		printf("Loading IPv6 routes\n");
	while (Loading6)
	{
		int len;
		char buf[65536];
		struct iovec iov = { buf, sizeof(buf) };
		struct sockaddr_nl sa;
		struct nlmsghdr *nh;

		struct msghdr msg = { &sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
		len = recvmsg(NLSOCK, &msg, 0);

		for (nh = (struct nlmsghdr *) buf; NLMSG_OK (nh, len);
			nh = NLMSG_NEXT (nh, len)) {
			if (nh->nlmsg_type == NLMSG_DONE)
			{
				Loading6=0;
				break;
			}
			parse_netlink(nh);
		}
	}
	request.hdr.nlmsg_seq = 2;
	request.msg.rtm_family = AF_INET;
	sendmsg(NLSOCK, (struct msghdr *) &rtnl_msg, 0);
	char Loading4=1;
	if (Debug)
		printf("Loading IPv4 routes\n");
	while (Loading4)
	{
		int len;
		char buf[65536];
		struct iovec iov = { buf, sizeof(buf) };
		struct sockaddr_nl sa;
		struct nlmsghdr *nh;

		struct msghdr msg = { &sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
		len = recvmsg(NLSOCK, &msg, 0);

		for (nh = (struct nlmsghdr *) buf; NLMSG_OK (nh, len);
			nh = NLMSG_NEXT (nh, len)) {
			if (nh->nlmsg_type == NLMSG_DONE)
			{
				Loading4=0;
				break;
			}
			parse_netlink(nh);
		}
	}
}

void update_router(void)
{
	struct timeval timeout={0};
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(NLSOCK, &read_fds);
	int FD_MAX = NLSOCK;
	if (inet_netof(node_IPv4) != INADDR_ANY)
	{
		FD_SET(ROUTER_SOCK4, &read_fds);
		FD_MAX = MAX(FD_MAX, ROUTER_SOCK4);
		struct RouterClient* Client = RouterClients4;
		while (Client)
		{
			router_flush_data(Client);
			Client=Client->Next;
		}
		router_poll_v4clients();
	}
	if (memcmp(&node_IPv6, &in6addr_any, sizeof(struct in6_addr)))
	{
		FD_SET(ROUTER_SOCK6, &read_fds);
		FD_MAX = MAX(FD_MAX, ROUTER_SOCK6);
		struct RouterClient* Client = RouterClients6;
		while (Client)
		{
			router_flush_data(Client);
			Client=Client->Next;
		}
		router_poll_v6clients();
	}
	while (select(FD_MAX+1, &read_fds, NULL, NULL, &timeout))
	{
		if (FD_ISSET(ROUTER_SOCK4, &read_fds))
		{
			struct sockaddr_in clientaddr;
			int clientlen = sizeof(clientaddr);
			int ClientSock = accept(ROUTER_SOCK4, (struct sockaddr *) &clientaddr, &clientlen);
			if (ClientSock >-1)
			{
				fcntl(ClientSock, F_SETFL, O_NONBLOCK);
				router_add_ipv4client(ClientSock);
			}
		}
		if (FD_ISSET(ROUTER_SOCK6, &read_fds))
		{
			struct sockaddr_in6 clientaddr;
			int clientlen = sizeof(clientaddr);
			int ClientSock = accept(ROUTER_SOCK6, (struct sockaddr *) &clientaddr, &clientlen);
			if (ClientSock >-1)
			{
				fcntl(ClientSock, F_SETFL, O_NONBLOCK);
				router_add_ipv6client(ClientSock);
			}
		}
		if (FD_ISSET(NLSOCK, &read_fds))
		{
			int len;
			char buf[65536];
			struct iovec iov = { buf, sizeof(buf) };
			struct sockaddr_nl sa;
			struct nlmsghdr *nh;

			struct msghdr msg = { &sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
			len = recvmsg(NLSOCK, &msg, 0);

			for (nh = (struct nlmsghdr *) buf; NLMSG_OK (nh, len);
				nh = NLMSG_NEXT (nh, len)) {
				if (nh->nlmsg_type == NLMSG_DONE)
				{
					break;
				}

				if (nh->nlmsg_type == NLMSG_ERROR)
				{
				       continue;
				}
				parse_netlink(nh);
			}

	/*		struct RouteEntry6* CurrentRoute6 = FirstRoute6;
			while(CurrentRoute6)
			{
				char    destination_address[72]={0};
				inet_ntop(AF_INET6, &CurrentRoute6->Route.dest, destination_address, sizeof(destination_address));
				printf("Route: %s/%d metric %u\n", destination_address, CurrentRoute6->Route.pl, CurrentRoute6->Route.metric);
				CurrentRoute6=CurrentRoute6->Next;
			}
			struct RouteEntry4* CurrentRoute4 = FirstRoute4;
			while(CurrentRoute4)
			{
				char    destination_address[72]={0};
				inet_ntop(AF_INET, &CurrentRoute4->Route.dest, destination_address, sizeof(destination_address));
				printf("Route: %s/%d metric %u\n", destination_address, CurrentRoute4->Route.pl, CurrentRoute4->Route.metric);
				CurrentRoute4=CurrentRoute4->Next;
			}
*/		}
	}
}
