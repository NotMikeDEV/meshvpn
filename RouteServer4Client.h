class RouteServer4Client : public SocketHandler {
private:
	int Socket;
	RouteServer4* Parent;
public:
	RouteServer4Client(int Socket, RouteServer4* Parent);
	~RouteServer4Client();
	void Send(unsigned char* Packet, int Length);
	int GetHandle();
	bool ReadHandle();
	void WriteHandle();
};