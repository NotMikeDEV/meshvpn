struct Route6 {
	struct in6_addr Destination;
	unsigned char PrefixLength;
	struct in6_addr Gateway;
	uint32_t OriginalMetric;
	uint32_t Metric;
	unsigned char RoutingTable;
	unsigned char Protocol;
	int InterfaceID;
};

class RouteServer6;
class NetLink6 : public SocketHandler {
private:
	int Socket;
	unsigned char RoutingTable;
	RouteServer6* Parent;
	int NL_SEQ;
public:
	std::vector<struct Route6> Routes;
	NetLink6(unsigned char RoutingTable);
	void AttachServer(RouteServer6* Parent);
	void Parse(struct nlmsghdr *nh);
	struct Route6 ParseRoute(struct nlmsghdr *nlh);
	int GetHandle();
	bool ReadHandle();
	void AddRoute(struct Route6 Route);
	void DeleteRoute(struct Route6 Route);
};