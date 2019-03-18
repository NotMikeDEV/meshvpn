#include "Globals.h"
struct nl_req_t {
	struct nlmsghdr hdr;
	struct rtmsg msg;
};
NetLink6::NetLink6(unsigned char RoutingTable)
{
	Socket = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
	struct sockaddr_nl nladdr = { 0 };
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = 0;
	nladdr.nl_groups = RTMGRP_IPV6_ROUTE | RTMGRP_LINK;
	bind(Socket, (struct sockaddr*)&nladdr, sizeof(nladdr));
	fcntl(Socket, F_SETFL, O_NONBLOCK);

	struct nl_req_t request = { 0 };
	request.hdr.nlmsg_len = sizeof(request);
	request.hdr.nlmsg_type = RTM_GETROUTE;
	request.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	request.hdr.nlmsg_seq = 1;
	request.hdr.nlmsg_pid = 0;
	request.msg.rtm_family = AF_INET6;
	request.msg.rtm_table = RoutingTable;
	struct iovec io = { 0 };
	io.iov_base = &request;
	io.iov_len = request.hdr.nlmsg_len;
	struct msghdr rtnl_msg = { 0 };
	rtnl_msg.msg_iov = &io;
	rtnl_msg.msg_iovlen = 1;
	sendmsg(Socket, (struct msghdr *) &rtnl_msg, 0);
	this->RoutingTable = RoutingTable;
}
void NetLink6::AttachServer(RouteServer6* Parent)
{
	this->Parent = Parent;
}
int NetLink6::GetHandle()
{
	return Socket;
}
struct Route6 NetLink6::ParseRoute(struct nlmsghdr *nlh)
{
	struct Route6 ROUTE = { 0 };
	struct  rtmsg* route_entry = (struct rtmsg *) NLMSG_DATA(nlh);

	if (route_entry->rtm_family != AF_INET6)
		return ROUTE;
	ROUTE.RoutingTable = route_entry->rtm_table;
	ROUTE.Protocol = route_entry->rtm_protocol;

	ROUTE.PrefixLength = route_entry->rtm_dst_len;
	struct  rtattr* route_attribute = (struct rtattr *) RTM_RTA(route_entry);
	int route_attribute_len = RTM_PAYLOAD(nlh);
	while (RTA_OK(route_attribute, route_attribute_len))
	{
		if (route_attribute->rta_type == RTA_DST)
		{
			memcpy(&ROUTE.Destination, RTA_DATA(route_attribute), sizeof(ROUTE.Destination));
		}
		if (route_attribute->rta_type == RTA_GATEWAY)
		{
			memcpy(&ROUTE.Gateway, RTA_DATA(route_attribute), sizeof(ROUTE.Gateway));
		}
		if (route_attribute->rta_type == RTA_PRIORITY)
		{
			ROUTE.Metric = *(uint32_t*)RTA_DATA(route_attribute);
		}
		if (route_attribute->rta_type == RTA_OIF)
		{
			ROUTE.InterfaceID = *(int*)RTA_DATA(route_attribute);
		}
		route_attribute = RTA_NEXT(route_attribute, route_attribute_len);
	}
	return ROUTE;
}
void NetLink6::Parse(struct nlmsghdr *nh)
{
	if (nh->nlmsg_type == RTM_DELLINK || nh->nlmsg_type == RTM_NEWLINK)
	{
		struct ifinfomsg* MSG = (struct ifinfomsg*) NLMSG_DATA(nh);
		if (nh->nlmsg_type == RTM_DELLINK || (nh->nlmsg_type == RTM_NEWLINK && !(MSG->ifi_flags&IFF_UP)))
		{
			if (Debug > 1)
				printf("Link Down %u\n", MSG->ifi_index);
			for (int index = 0; index < Routes.size(); ++index)
			{
				if (Routes[index].InterfaceID == MSG->ifi_index)
				{
					if (Debug > 1)
					{
						char IP[128];
						inet_ntop(AF_INET6, &Routes[index].Destination, IP, sizeof(IP));
						char Gateway[128];
						inet_ntop(AF_INET6, &Routes[index].Gateway, Gateway, sizeof(Gateway));
						printf("Delete Route to %s/%u via %s (Metric %u, Table %u, Proto %u, Interface %u)\n", IP, Routes[index].PrefixLength, Gateway, Routes[index].Metric, Routes[index].RoutingTable, Routes[index].Protocol, Routes[index].InterfaceID);
					}
					Parent->WithdrawRoute(&Routes[index].Destination, Routes[index].PrefixLength, Routes[index].Metric);
					Routes.erase(Routes.begin() + index);
					index--;
				}
			}
		}
	}
	if (nh->nlmsg_type == RTM_NEWROUTE)
	{
		struct Route6 Route = ParseRoute(nh);
		if (Route.RoutingTable != RoutingTable)
			return;
		if (Route.Protocol == ROUTING_PROTOCOL)
			return;
		for (struct Route6 &R : Routes)
		{
			if (memcmp(&R.Destination, &Route.Destination, sizeof(Route.Destination)) == 0 && R.PrefixLength == Route.PrefixLength && R.Metric == Route.Metric)
				return;
		}
		if (Debug > 1)
		{
			char IP[128];
			inet_ntop(AF_INET6, &Route.Destination, IP, sizeof(IP));
			char Gateway[128];
			inet_ntop(AF_INET6, &Route.Gateway, Gateway, sizeof(Gateway));
			printf("New Route to %s/%u via %s (Metric %u, Table %u, Proto %u, Interface %u)\n", IP, Route.PrefixLength, Gateway, Route.Metric, Route.RoutingTable, Route.Protocol, Route.InterfaceID);
		}
		Parent->AdvertiseRoute(&Route.Destination, Route.PrefixLength, Route.Metric);
		Routes.push_back(Route);
	}
	if (nh->nlmsg_type == RTM_DELROUTE)
	{
		struct Route6 Route = ParseRoute(nh);
		if (Route.RoutingTable != RoutingTable)
			return;
		if (Route.Protocol == ROUTING_PROTOCOL)
			return;
		for (int index = 0; index < Routes.size(); ++index)
		{
			if (memcmp(&Routes[index].Destination, &Route.Destination, sizeof(Route.Destination)) == 0 && Routes[index].PrefixLength == Route.PrefixLength && Routes[index].Metric == Route.Metric)
			{
				Routes.erase(Routes.begin() + index);
				if (Debug > 1)
				{
					char IP[128];
					inet_ntop(AF_INET6, &Route.Destination, IP, sizeof(IP));
					char Gateway[128];
					inet_ntop(AF_INET6, &Route.Gateway, Gateway, sizeof(Gateway));
					printf("Delete Route to %s/%u via %s (Metric %u, Table %u, Proto %u, Interface %u)\n", IP, Route.PrefixLength, Gateway, Route.Metric, Route.RoutingTable, Route.Protocol, Route.InterfaceID);
				}
				Parent->WithdrawRoute(&Route.Destination, Route.PrefixLength, Route.Metric);
			}
		}
	}
}
bool NetLink6::ReadHandle()
{
	char buf[65536];
	struct iovec iov = { buf, sizeof(buf) };
	struct sockaddr_nl sa;
	struct nlmsghdr *nh;

	struct msghdr msg = { &sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
	int len = recvmsg(Socket, &msg, 0);
	if (len > 0)
	{
		for (nh = (struct nlmsghdr *) buf; NLMSG_OK(nh, len); nh = NLMSG_NEXT(nh, len))
		{
			Parse(nh);
		}
		return true;
	}
	return false;
}
void NetLink6::AddRoute(struct Route6 Route)
{
	char buffer[1024] = { 0 };
	struct nlmsghdr* hdr = (struct nlmsghdr*)buffer;
	hdr->nlmsg_len = sizeof(struct nlmsghdr);
	hdr->nlmsg_type = RTM_NEWROUTE;
	hdr->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | 0x600;
	hdr->nlmsg_seq = NL_SEQ++;
	hdr->nlmsg_pid = 0;
	struct rtmsg* msg = (struct rtmsg*)(buffer + hdr->nlmsg_len);
	msg->rtm_family = AF_INET6;
	msg->rtm_table = RoutingTable;
	msg->rtm_protocol = ROUTING_PROTOCOL;
	msg->rtm_dst_len = Route.PrefixLength;
	msg->rtm_type = RTN_UNICAST;
	hdr->nlmsg_len += sizeof(struct rtmsg);

	struct rtattr *rta = (struct rtattr *)(buffer + hdr->nlmsg_len);
	rta->rta_type = RTA_DST;
	rta->rta_len = RTA_LENGTH(sizeof(struct in6_addr));
	memcpy(RTA_DATA(rta), &Route.Destination, sizeof(struct in6_addr));
	hdr->nlmsg_len += rta->rta_len;

	rta = (struct rtattr *)(buffer + hdr->nlmsg_len);
	rta->rta_type = RTA_GATEWAY;
	rta->rta_len = RTA_LENGTH(sizeof(struct in6_addr));
	memcpy(RTA_DATA(rta), &Route.Gateway, sizeof(struct in6_addr));
	hdr->nlmsg_len += rta->rta_len;

	rta = (struct rtattr *)(buffer + hdr->nlmsg_len);
	rta->rta_type = RTA_PRIORITY;
	rta->rta_len = RTA_LENGTH(sizeof(int));
	memcpy(RTA_DATA(rta), &Route.Metric, sizeof(int));
	hdr->nlmsg_len += rta->rta_len;

	struct iovec io = { 0 };
	io.iov_base = &buffer;
	io.iov_len = hdr->nlmsg_len;
	struct msghdr rtnl_msg = { 0 };
	struct sockaddr_nl addr = { 0 };
	addr.nl_family = AF_NETLINK;
	rtnl_msg.msg_name = &addr;
	rtnl_msg.msg_namelen = sizeof(addr);
	rtnl_msg.msg_iov = &io;
	rtnl_msg.msg_iovlen = 1;
	sendmsg(Socket, (struct msghdr *) &rtnl_msg, 0);
}
void NetLink6::DeleteRoute(struct Route6 Route)
{
	char buffer[1024] = { 0 };
	struct nlmsghdr* hdr = (struct nlmsghdr*)buffer;
	hdr->nlmsg_len = sizeof(struct nlmsghdr);
	hdr->nlmsg_type = RTM_DELROUTE;
	hdr->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | 0x600;
	hdr->nlmsg_seq = NL_SEQ++;
	hdr->nlmsg_pid = 0;
	struct rtmsg* msg = (struct rtmsg*)(buffer + hdr->nlmsg_len);
	msg->rtm_family = AF_INET6;
	msg->rtm_table = RoutingTable;
	msg->rtm_protocol = ROUTING_PROTOCOL;
	msg->rtm_dst_len = Route.PrefixLength;
	msg->rtm_type = RTN_UNICAST;
	hdr->nlmsg_len += sizeof(struct rtmsg);

	struct rtattr *rta = (struct rtattr *)(buffer + hdr->nlmsg_len);
	rta->rta_type = RTA_DST;
	rta->rta_len = RTA_LENGTH(sizeof(struct in6_addr));
	memcpy(RTA_DATA(rta), &Route.Destination, sizeof(struct in6_addr));
	hdr->nlmsg_len += rta->rta_len;

	rta = (struct rtattr *)(buffer + hdr->nlmsg_len);
	rta->rta_type = RTA_GATEWAY;
	rta->rta_len = RTA_LENGTH(sizeof(struct in6_addr));
	memcpy(RTA_DATA(rta), &Route.Gateway, sizeof(struct in6_addr));
	hdr->nlmsg_len += rta->rta_len;

	rta = (struct rtattr *)(buffer + hdr->nlmsg_len);
	rta->rta_type = RTA_PRIORITY;
	rta->rta_len = RTA_LENGTH(sizeof(int));
	memcpy(RTA_DATA(rta), &Route.Metric, sizeof(int));
	hdr->nlmsg_len += rta->rta_len;

	struct iovec io = { 0 };
	io.iov_base = &buffer;
	io.iov_len = hdr->nlmsg_len;
	struct msghdr rtnl_msg = { 0 };
	struct sockaddr_nl addr = { 0 };
	addr.nl_family = AF_NETLINK;
	rtnl_msg.msg_name = &addr;
	rtnl_msg.msg_namelen = sizeof(addr);
	rtnl_msg.msg_iov = &io;
	rtnl_msg.msg_iovlen = 1;
	sendmsg(Socket, (struct msghdr *) &rtnl_msg, 0);
}