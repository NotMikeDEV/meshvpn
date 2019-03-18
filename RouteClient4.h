class RouteClient4 : SocketHandler {
private:
	MeshVPN* app;
	RemoteNode* Parent;
	struct in_addr IPv4;
	unsigned short Port;
	int Socket;
	unsigned char RecvBuffer[1 + 4 + 1 + 4];
	int RecvOffset=0;
	std::vector<struct Route4> Routes;
public:
	RouteClient4(MeshVPN* app, RemoteNode* Parent, struct in_addr* IPv4, unsigned short Port);
	~RouteClient4();
	void Reconnect();
	int GetHandle();
	bool ReadHandle();
};