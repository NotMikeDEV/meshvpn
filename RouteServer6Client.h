class RouteServer6Client : public SocketHandler {
private:
	int Socket;
	RouteServer6* Parent;
public:
	RouteServer6Client(int Socket, RouteServer6* Parent);
	~RouteServer6Client();
	void Send(unsigned char* Packet, int Length);
	int GetHandle();
	bool ReadHandle();
	void WriteHandle();
};