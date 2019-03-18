class TunTap;
class MeshVPN;
class RouteServer4;
class RouteServer6;

class Switch : public SocketHandler{
private:
	TunTap* Interface;
	int Socket;
	unsigned char NodeKey[NODE_KEY_LENGTH];
	MeshVPN* app;
	struct in_addr IPv4;
	unsigned char IPv4_PrefixLength;
	struct in6_addr IPv6;
	unsigned char IPv6_PrefixLength;
	RouteServer4* RS4 = NULL;
	RouteServer6* RS6 = NULL;
public:
	NetLink4* NL4 = NULL;
	NetLink6* NL6 = NULL;
	int RoutingTable = -1;
	unsigned short Port;
	Switch(MeshVPN* app);
	~Switch();
	void Init(char* InterfaceName, unsigned short Port, int RoutingTable);
	void SetIPv4(struct in_addr* IP, int PrefixLength);
	void SetIPv6(struct in6_addr* IP, int PrefixLength);
	struct in_addr* GetIPv4();
	struct in6_addr* GetIPv6();
	std::vector<RemoteNode*> RemoteNodes;
	int GetHandle();
	bool ReadHandle();
	void RunInterval();
	void AddRemote(struct sockaddr_in6* Address, bool Static);
	RemoteNode* GetRemote(struct sockaddr_in6* Address);
	RemoteNode* GetRemote(unsigned char* NodeKey);
	void RemoveRemote(struct sockaddr_in6* Address);
	void SendRemote(RemoteNode* Node, unsigned char Type, unsigned char* Packet, int Length);
	void ParsePacket(unsigned char Type, unsigned char* FromKey, unsigned char* ToKey, unsigned char* Payload, int Length, struct sockaddr_in6* Address);
	void LocalEthernetPacket(unsigned char* Packet, int Length);
	void RemoteEthernetPacket(RemoteNode* Node, unsigned char* Packet, int Length);
};