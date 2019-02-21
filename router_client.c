#include "globals.h"
int NL_SEQ=1;
void netlink_add_IPv4(struct in_addr* dest, unsigned char pl, struct in_addr* gateway, int metric)
{
	char buffer[1024]={0};
	struct nlmsghdr* hdr = (struct nlmsghdr*)buffer;
	hdr->nlmsg_len = sizeof(struct nlmsghdr);
	hdr->nlmsg_type = RTM_NEWROUTE;
	hdr->nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK|0x600;
	hdr->nlmsg_seq = NL_SEQ++;
	hdr->nlmsg_pid = 0;
	struct rtmsg* msg = (struct rtmsg*)(buffer+hdr->nlmsg_len);
	msg->rtm_family = AF_INET;
	msg->rtm_table = ROUTE_TABLE;
	msg->rtm_protocol = ROUTE_PROTO;
	msg->rtm_dst_len = pl;
	msg->rtm_type = RTN_UNICAST;
	hdr->nlmsg_len += sizeof(struct rtmsg);

	struct rtattr *rta = (struct rtattr *)(buffer+hdr->nlmsg_len);
	rta->rta_type = RTA_DST;
	rta->rta_len = RTA_LENGTH(sizeof(struct in_addr));
	memcpy(RTA_DATA(rta), dest, sizeof(struct in_addr));
	hdr->nlmsg_len += rta->rta_len;

	rta = (struct rtattr *)(buffer+hdr->nlmsg_len);
	rta->rta_type = RTA_GATEWAY;
	rta->rta_len = RTA_LENGTH(sizeof(struct in_addr));
	memcpy(RTA_DATA(rta), gateway, sizeof(struct in_addr));
	hdr->nlmsg_len += rta->rta_len;

	rta = (struct rtattr *)(buffer+hdr->nlmsg_len);
	rta->rta_type = RTA_PRIORITY;
	rta->rta_len = RTA_LENGTH(sizeof(int));
	memcpy(RTA_DATA(rta), &metric, sizeof(int));
	hdr->nlmsg_len += rta->rta_len;

	struct iovec io={0};
	io.iov_base = &buffer;
	io.iov_len = hdr->nlmsg_len;
	struct msghdr rtnl_msg={0};
	struct sockaddr_nl addr={0};
	addr.nl_family=AF_NETLINK;
	rtnl_msg.msg_name = &addr;
	rtnl_msg.msg_namelen = sizeof(addr);
	rtnl_msg.msg_iov = &io;
	rtnl_msg.msg_iovlen = 1;
	sendmsg(NLSOCK, (struct msghdr *) &rtnl_msg, 0);
	if (Debug > 1)
	{
		char Target[72];
		char Gateway[72];
		inet_ntop(AF_INET, dest, Target, sizeof(Target));
		inet_ntop(AF_INET, gateway, Gateway, sizeof(Gateway));
		printf("Added route %s/%u via %s (prio %u)\n", Target, pl, Gateway, metric);
	}
}
void netlink_del_IPv4(struct in_addr* dest, unsigned char pl, struct in_addr* gateway, int metric)
{
	char buffer[1024]={0};
	struct nlmsghdr* hdr = (struct nlmsghdr*)buffer;
	hdr->nlmsg_len = sizeof(struct nlmsghdr);
	hdr->nlmsg_type = RTM_DELROUTE;
	hdr->nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK;
	hdr->nlmsg_seq = NL_SEQ++;
	hdr->nlmsg_pid = 0;
	struct rtmsg* msg = (struct rtmsg*)(buffer+hdr->nlmsg_len);
	msg->rtm_family = AF_INET;
	msg->rtm_table = ROUTE_TABLE;
	msg->rtm_protocol = ROUTE_PROTO;
	msg->rtm_dst_len = pl;
	msg->rtm_type = RTN_UNICAST;
	hdr->nlmsg_len += sizeof(struct rtmsg);

	struct rtattr *rta = (struct rtattr *)(buffer+hdr->nlmsg_len);
	rta->rta_type = RTA_DST;
	rta->rta_len = RTA_LENGTH(sizeof(struct in_addr));
	memcpy(RTA_DATA(rta), dest, sizeof(struct in_addr));
	hdr->nlmsg_len += rta->rta_len;

	rta = (struct rtattr *)(buffer+hdr->nlmsg_len);
	rta->rta_type = RTA_GATEWAY;
	rta->rta_len = RTA_LENGTH(sizeof(struct in_addr));
	memcpy(RTA_DATA(rta), gateway, sizeof(struct in_addr));
	hdr->nlmsg_len += rta->rta_len;

	rta = (struct rtattr *)(buffer+hdr->nlmsg_len);
	rta->rta_type = RTA_PRIORITY;
	rta->rta_len = RTA_LENGTH(sizeof(int));
	memcpy(RTA_DATA(rta), &metric, sizeof(int));
	hdr->nlmsg_len += rta->rta_len;

	struct iovec io={0};
	io.iov_base = &buffer;
	io.iov_len = hdr->nlmsg_len;
	struct msghdr rtnl_msg={0};
	struct sockaddr_nl addr={0};
	addr.nl_family=AF_NETLINK;
	rtnl_msg.msg_name = &addr;
	rtnl_msg.msg_namelen = sizeof(addr);
	rtnl_msg.msg_iov = &io;
	rtnl_msg.msg_iovlen = 1;
	sendmsg(NLSOCK, (struct msghdr *) &rtnl_msg, 0);
	if (Debug > 1)
	{
		char Target[72];
		char Gateway[72];
		inet_ntop(AF_INET, dest, Target, sizeof(Target));
		inet_ntop(AF_INET, gateway, Gateway, sizeof(Gateway));
		printf("Deleted route %s/%u via %s (prio %u)\n", Target, pl, Gateway, metric);
	}
}
void netlink_add_IPv6(struct in6_addr* dest, unsigned char pl, struct in6_addr* gateway, int metric)
{
	char buffer[1024]={0};
	struct nlmsghdr* hdr = (struct nlmsghdr*)buffer;
	hdr->nlmsg_len = sizeof(struct nlmsghdr);
	hdr->nlmsg_type = RTM_NEWROUTE;
	hdr->nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK|0x600;
	hdr->nlmsg_seq = NL_SEQ++;
	hdr->nlmsg_pid = 0;
	struct rtmsg* msg = (struct rtmsg*)(buffer+hdr->nlmsg_len);
	msg->rtm_family = AF_INET6;
	msg->rtm_table = ROUTE_TABLE;
	msg->rtm_protocol = ROUTE_PROTO;
	msg->rtm_dst_len = pl;
	msg->rtm_type = RTN_UNICAST;
	hdr->nlmsg_len += sizeof(struct rtmsg);

	struct rtattr *rta = (struct rtattr *)(buffer+hdr->nlmsg_len);
	rta->rta_type = RTA_DST;
	rta->rta_len = RTA_LENGTH(sizeof(struct in6_addr));
	memcpy(RTA_DATA(rta), dest, sizeof(struct in6_addr));
	hdr->nlmsg_len += rta->rta_len;

	rta = (struct rtattr *)(buffer+hdr->nlmsg_len);
	rta->rta_type = RTA_GATEWAY;
	rta->rta_len = RTA_LENGTH(sizeof(struct in6_addr));
	memcpy(RTA_DATA(rta), gateway, sizeof(struct in6_addr));
	hdr->nlmsg_len += rta->rta_len;

	rta = (struct rtattr *)(buffer+hdr->nlmsg_len);
	rta->rta_type = RTA_PRIORITY;
	rta->rta_len = RTA_LENGTH(sizeof(int));
	memcpy(RTA_DATA(rta), &metric, sizeof(int));
	hdr->nlmsg_len += rta->rta_len;

	struct iovec io={0};
	io.iov_base = &buffer;
	io.iov_len = hdr->nlmsg_len;
	struct msghdr rtnl_msg={0};
	struct sockaddr_nl addr={0};
	addr.nl_family=AF_NETLINK;
	rtnl_msg.msg_name = &addr;
	rtnl_msg.msg_namelen = sizeof(addr);
	rtnl_msg.msg_iov = &io;
	rtnl_msg.msg_iovlen = 1;
	sendmsg(NLSOCK, (struct msghdr *) &rtnl_msg, 0);
	if (Debug > 1)
	{
		char Target[192];
		char Gateway[192];
		inet_ntop(AF_INET6, dest, Target, sizeof(Target));
		inet_ntop(AF_INET6, gateway, Gateway, sizeof(Gateway));
		printf("Added route %s/%u via %s (prio %u)\n", Target, pl, Gateway, metric);
	}
}
void netlink_del_IPv6(struct in6_addr* dest, unsigned char pl, struct in6_addr* gateway, int metric)
{
	char buffer[1024]={0};
	struct nlmsghdr* hdr = (struct nlmsghdr*)buffer;
	hdr->nlmsg_len = sizeof(struct nlmsghdr);
	hdr->nlmsg_type = RTM_DELROUTE;
	hdr->nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK;
	hdr->nlmsg_seq = NL_SEQ++;
	hdr->nlmsg_pid = 0;
	struct rtmsg* msg = (struct rtmsg*)(buffer+hdr->nlmsg_len);
	msg->rtm_family = AF_INET6;
	msg->rtm_table = ROUTE_TABLE;
	msg->rtm_protocol = ROUTE_PROTO;
	msg->rtm_dst_len = pl;
	msg->rtm_type = RTN_UNICAST;
	hdr->nlmsg_len += sizeof(struct rtmsg);

	struct rtattr *rta = (struct rtattr *)(buffer+hdr->nlmsg_len);
	rta->rta_type = RTA_DST;
	rta->rta_len = RTA_LENGTH(sizeof(struct in6_addr));
	memcpy(RTA_DATA(rta), dest, sizeof(struct in6_addr));
	hdr->nlmsg_len += rta->rta_len;

	rta = (struct rtattr *)(buffer+hdr->nlmsg_len);
	rta->rta_type = RTA_GATEWAY;
	rta->rta_len = RTA_LENGTH(sizeof(struct in6_addr));
	memcpy(RTA_DATA(rta), gateway, sizeof(struct in6_addr));
	hdr->nlmsg_len += rta->rta_len;

	rta = (struct rtattr *)(buffer+hdr->nlmsg_len);
	rta->rta_type = RTA_PRIORITY;
	rta->rta_len = RTA_LENGTH(sizeof(int));
	memcpy(RTA_DATA(rta), &metric, sizeof(int));
	hdr->nlmsg_len += rta->rta_len;

	struct iovec io={0};
	io.iov_base = &buffer;
	io.iov_len = hdr->nlmsg_len;
	struct msghdr rtnl_msg={0};
	struct sockaddr_nl addr={0};
	addr.nl_family=AF_NETLINK;
	rtnl_msg.msg_name = &addr;
	rtnl_msg.msg_namelen = sizeof(addr);
	rtnl_msg.msg_iov = &io;
	rtnl_msg.msg_iovlen = 1;
	sendmsg(NLSOCK, (struct msghdr *) &rtnl_msg, 0);
	if (Debug > 1)
	{
		char Target[192];
		char Gateway[192];
		inet_ntop(AF_INET6, dest, Target, sizeof(Target));
		inet_ntop(AF_INET6, gateway, Gateway, sizeof(Gateway));
		printf("Deleted route %s/%u via %s (prio %u)\n", Target, pl, Gateway, metric);
	}
}

void router_client_poll(struct Node* Node)
{
	if (inet_netof(node_IPv4) != INADDR_ANY && inet_netof(Node->IPv4) != INADDR_ANY)
	{
		if (Node->RouterConn4.State == 0)
		{
			Node->RouterConn4.Socket = socket(AF_INET, SOCK_STREAM, 0);
			fcntl(Node->RouterConn4.Socket, F_SETFL, O_NONBLOCK);
			struct sockaddr_in remote={0};
			remote.sin_family = AF_INET;
			memcpy(&remote.sin_addr, &Node->IPv4, sizeof(struct in_addr));
			remote.sin_port = Node->address.sin6_port;

			connect(Node->RouterConn4.Socket, &remote, sizeof(remote));
			int KeepAlive=1;
			int keepcnt = 3;
			int keepidle = 15;
			int keepintvl = 120;
			setsockopt(Node->RouterConn4.Socket, SOL_SOCKET, SO_KEEPALIVE, &KeepAlive, sizeof(KeepAlive));
			setsockopt(Node->RouterConn4.Socket, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int));
			setsockopt(Node->RouterConn4.Socket, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int));
			setsockopt(Node->RouterConn4.Socket, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(int));
			Node->RouterConn4.State = 1;
		}
		if (Node->RouterConn4.State == 1)
		{
			int ret = recv(Node->RouterConn4.Socket, Node->RouterConn4.RecvBuffer + Node->RouterConn4.RecvOffset, sizeof(struct route4)-Node->RouterConn4.RecvOffset, 0);
			while (ret > 0)
			{
				Node->RouterConn4.RecvOffset += ret;
				if (Node->RouterConn4.RecvOffset == sizeof(struct route4))
				{
					Node->RouterConn4.RecvOffset=0;
					struct route4* Route = (struct route4*)Node->RouterConn4.RecvBuffer;
					if (Route->type == 1)
					{
						unsigned char GotRouteAlready = 0;
						struct RouteEntry4* CurrentRoute = Node->RouterConn4.RouteList;
						while (CurrentRoute)
						{
							if (memcmp(&CurrentRoute->Route.dest, &Route->dest, sizeof(Route->dest))==0 && CurrentRoute->Route.pl == Route->pl && CurrentRoute->Route.metric == Route->metric)
								GotRouteAlready = 1;
							CurrentRoute = CurrentRoute->Next;
						}
						if (!GotRouteAlready)
						{
							CurrentRoute = (struct RouteEntry4*)malloc(sizeof(struct RouteEntry4));
							memset(CurrentRoute,0,sizeof(struct RouteEntry4));
							memcpy(&CurrentRoute->Route, Route, sizeof(CurrentRoute->Route));
							CurrentRoute->Next = Node->RouterConn4.RouteList;
							if (CurrentRoute->Next)
								CurrentRoute->Next->Previous=CurrentRoute;
							Node->RouterConn4.RouteList = CurrentRoute;
							netlink_add_IPv4(&Route->dest, Route->pl, &Node->IPv4, Route->metric + Node->Latency);
							if (Debug)
								printf("Add IPv4 Route\n");
						}
					}
					if (Route->type == 0)
					{
						struct RouteEntry4* CurrentRoute = Node->RouterConn4.RouteList;
						while (CurrentRoute)
						{
							struct RouteEntry4* NextRoute = CurrentRoute->Next;
							if (memcmp(&CurrentRoute->Route.dest, &Route->dest, sizeof(Route->dest))==0 && CurrentRoute->Route.pl == Route->pl && CurrentRoute->Route.metric == Route->metric)
							{
								if (Debug)
									printf("Delete IPv4 Route\n");
								netlink_del_IPv4(&Route->dest, Route->pl, &Node->IPv4, Route->metric + Node->Latency);
								if (CurrentRoute->Next)
									CurrentRoute->Next->Previous=CurrentRoute->Previous;
								if (CurrentRoute->Previous)
									CurrentRoute->Previous->Next=CurrentRoute->Next;
								if (CurrentRoute == Node->RouterConn4.RouteList)
									Node->RouterConn4.RouteList = CurrentRoute->Next;
								free(CurrentRoute);
							}
							CurrentRoute = NextRoute;
						}
					}
				}
				ret = recv(Node->RouterConn4.Socket, Node->RouterConn4.RecvBuffer + Node->RouterConn4.RecvOffset, sizeof(struct route4)-Node->RouterConn4.RecvOffset, 0);
			}
			if (ret == 0 || Node->State == 0)
			{
				struct RouteEntry4* CurrentRoute = Node->RouterConn4.RouteList;
				while (CurrentRoute)
				{
					struct RouteEntry4* NextRoute = CurrentRoute->Next;
					if (Debug)
						printf("Delete IPv4 Route (Node Disconnect)\n");
					netlink_del_IPv4(&CurrentRoute->Route.dest, CurrentRoute->Route.pl, &Node->IPv4, CurrentRoute->Route.metric + Node->Latency);
					free(CurrentRoute);
					CurrentRoute = NextRoute;
				}
				Node->RouterConn4.RouteList = NULL;
				close(Node->RouterConn4.Socket);
				Node->RouterConn4.Socket=-1;
				Node->RouterConn4.State=0;
			}
		}
	}
	if (memcmp(&node_IPv6, &in6addr_any, sizeof(struct in6_addr)) && memcmp(&Node->IPv6, &in6addr_any, sizeof(struct in6_addr)))
	{
		if (Node->RouterConn6.State == 0)
		{
			Node->RouterConn6.Socket = socket(AF_INET6, SOCK_STREAM, 0);
			fcntl(Node->RouterConn6.Socket, F_SETFL, O_NONBLOCK);
			struct sockaddr_in6 remote={0};
			remote.sin6_family = AF_INET6;
			memcpy(&remote.sin6_addr, &Node->IPv6, sizeof(struct in6_addr));
			remote.sin6_port = Node->address.sin6_port;

			connect(Node->RouterConn6.Socket, &remote, sizeof(remote));
			int KeepAlive=1;
			int keepcnt = 3;
			int keepidle = 15;
			int keepintvl = 120;
			setsockopt(Node->RouterConn6.Socket, SOL_SOCKET, SO_KEEPALIVE, &KeepAlive, sizeof(KeepAlive));
			setsockopt(Node->RouterConn6.Socket, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int));
			setsockopt(Node->RouterConn6.Socket, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int));
			setsockopt(Node->RouterConn6.Socket, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(int));
			Node->RouterConn6.State = 1;
		}
		if (Node->RouterConn6.State == 1)
		{
			int ret = recv(Node->RouterConn6.Socket, Node->RouterConn6.RecvBuffer + Node->RouterConn6.RecvOffset, sizeof(struct route6)-Node->RouterConn6.RecvOffset, 0);
			while (ret > 0)
			{
				Node->RouterConn6.RecvOffset += ret;
				if (Node->RouterConn6.RecvOffset == sizeof(struct route6))
				{
					Node->RouterConn6.RecvOffset=0;
					struct route6* Route = (struct route6*)Node->RouterConn6.RecvBuffer;
					if (Route->type == 1)
					{
						unsigned char GotRouteAlready = 0;
						struct RouteEntry6* CurrentRoute = Node->RouterConn6.RouteList;
						while (CurrentRoute)
						{
							if (memcmp(&CurrentRoute->Route.dest, &Route->dest, sizeof(Route->dest))==0 && CurrentRoute->Route.pl == Route->pl && CurrentRoute->Route.metric == Route->metric)
								GotRouteAlready = 1;
							CurrentRoute = CurrentRoute->Next;
						}
						if (!GotRouteAlready)
						{
							CurrentRoute = (struct RouteEntry6*)malloc(sizeof(struct RouteEntry6));
							memset(CurrentRoute,0,sizeof(struct RouteEntry6));
							memcpy(&CurrentRoute->Route, Route, sizeof(CurrentRoute->Route));
							CurrentRoute->Next = Node->RouterConn6.RouteList;
							if (CurrentRoute->Next)
								CurrentRoute->Next->Previous=CurrentRoute;
							Node->RouterConn6.RouteList = CurrentRoute;
							netlink_add_IPv6(&Route->dest, Route->pl, &Node->IPv6, Route->metric + Node->Latency);
							if (Debug)
								printf("Add IPv6 Route\n");
						}
					}
					if (Route->type == 0)
					{
						struct RouteEntry6* CurrentRoute = Node->RouterConn6.RouteList;
						while (CurrentRoute)
						{
							struct RouteEntry6* NextRoute = CurrentRoute->Next;
							if (memcmp(&CurrentRoute->Route.dest, &Route->dest, sizeof(Route->dest))==0 && CurrentRoute->Route.pl == Route->pl && CurrentRoute->Route.metric == Route->metric)
							{
								if (Debug)
									printf("Delete IPv6 Route\n");
								netlink_del_IPv6(&Route->dest, Route->pl, &Node->IPv6, Route->metric + Node->Latency);
								if (CurrentRoute->Next)
									CurrentRoute->Next->Previous=CurrentRoute->Previous;
								if (CurrentRoute->Previous)
									CurrentRoute->Previous->Next=CurrentRoute->Next;
								if (CurrentRoute == Node->RouterConn6.RouteList)
									Node->RouterConn6.RouteList = CurrentRoute->Next;
								free(CurrentRoute);
							}
							CurrentRoute = NextRoute;
						}
					}
				}
				ret = recv(Node->RouterConn6.Socket, Node->RouterConn6.RecvBuffer + Node->RouterConn6.RecvOffset, sizeof(struct route6)-Node->RouterConn6.RecvOffset, 0);
			}
			if (ret == 0 || Node->State == 0)
			{
				struct RouteEntry6* CurrentRoute = Node->RouterConn6.RouteList;
				while (CurrentRoute)
				{
					struct RouteEntry6* NextRoute = CurrentRoute->Next;
					if (Debug)
						printf("Delete IPv6 Route (Node Disconnect)\n");
					netlink_del_IPv6(&CurrentRoute->Route.dest, CurrentRoute->Route.pl, &Node->IPv6, CurrentRoute->Route.metric + Node->Latency);
					free(CurrentRoute);
					CurrentRoute = NextRoute;
				}
				Node->RouterConn6.RouteList = NULL;
				close(Node->RouterConn6.Socket);
				Node->RouterConn6.Socket=-1;
				Node->RouterConn6.State=0;
			}
		}
	}
}
void router_client_poll_all()
{
	struct Node* Node=FirstNode;
	while (Node)
	{
		router_client_poll(Node);
		Node=Node->Next;
	}
}