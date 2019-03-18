struct MAC {
	unsigned char a[6];
};
class Switch;
class MeshVPN;
class RouteClient4;
class RouteClient6;
class NetLink4;
class NetLink6;
class RemoteNode {
	unsigned char Status = 0;
	int Retries = 0;
	struct in_addr IPv4;
	struct in6_addr IPv6;
	MeshVPN* app;
	RouteClient4* RC4 = NULL;
	RouteClient6* RC6 = NULL;
public:
	Switch* ParentSwitch;
	RemoteNode(struct sockaddr_in6* address, Switch* Switch, MeshVPN* app);
	~RemoteNode();
	unsigned char GetStatus();
	unsigned char GetRetries();
	void IncrementRetryCount();
	void SetStatus(unsigned char Status);
	void SetIPv4(struct in_addr* IP);
	void SetIPv6(struct in6_addr* IP);
	struct in_addr* GetIPv4();
	struct in6_addr* GetIPv6();
	std::vector<MAC> MACs;
	unsigned char NodeKey[NODE_KEY_LENGTH];
	struct timeval PingSendTime;
	unsigned int Latency;
	time_t NextSend;
	time_t LastRecv;
	struct sockaddr_in6 Address;
	bool Static;
	void DoNextSend(time_t Time);
};