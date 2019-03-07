unsigned char ROUTE_TABLE;
unsigned char ROUTE_PROTO;
int NLSOCK;
int ROUTER_SOCK4;
int ROUTER_SOCK6;
struct route4{
	uint32_t metric;
	struct in_addr dest;
	unsigned char pl;
	unsigned char type;
	int interface;
};
struct route6{
	uint32_t metric;
	struct in6_addr dest;
	unsigned char pl;
	unsigned char type;
	int interface;
};
struct RouteEntry4{
	struct route4 Route;
	struct RouteEntry4* Previous;
	struct RouteEntry4* Next;
};
struct RouteEntry4* FirstRoute4;
struct RouteEntry6{
	struct route6 Route;
	struct RouteEntry6* Previous;
	struct RouteEntry6* Next;
};
struct RouteEntry6* FirstRoute6;
struct RouterClient{
	int Socket;
	int SendBufferLength;
	unsigned char* SendBuffer;
	struct RouterClient* Previous;
	struct RouterClient* Next;
};
struct RouterClient* RouterClients4;
struct RouterClient* RouterClients6;
void init_router();
void update_router();
struct nl_req_t {
  struct nlmsghdr hdr;
  struct rtmsg msg;
};
