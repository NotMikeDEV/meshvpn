class RouteClient6 : SocketHandler {
private:
	MeshVPN* app;
	RemoteNode* Parent;
	struct in6_addr IPv6;
	unsigned short Port;
	int Socket;
	unsigned char RecvBuffer[1 + 16 + 1 + 4];
	int RecvOffset = 0;
	std::vector<struct Route6> Routes;
public:
	RouteClient6(MeshVPN* app, RemoteNode* Parent, struct in6_addr* IPv6, unsigned short Port);
	~RouteClient6();
	void Reconnect();
	int GetHandle();
	bool ReadHandle();
}; 
