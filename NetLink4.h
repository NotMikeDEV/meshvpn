struct Route4 {
	struct in_addr Destination;
	unsigned char PrefixLength;
	struct in_addr Gateway;
	uint32_t OriginalMetric;
	uint32_t Metric;
	unsigned char RoutingTable;
	unsigned char Protocol;
	int InterfaceID;
};

class RouteServer4;
class NetLink4 : public SocketHandler {
private:
	int Socket;
	unsigned char RoutingTable;
	RouteServer4* Parent;
	int NL_SEQ;
public:
	std::vector<struct Route4> Routes;
	NetLink4(unsigned char RoutingTable);
	void AttachServer(RouteServer4* Parent);
	void Parse(struct nlmsghdr *nh);
	struct Route4 ParseRoute(struct nlmsghdr *nlh);
	int GetHandle();
	bool ReadHandle();
	void AddRoute(struct Route4 Route);
	void DeleteRoute(struct Route4 Route);
};