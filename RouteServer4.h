#include "NetLink4.h"
#include "RouteServer4Client.h"

class RouteServer4 :public SocketHandler{
private:
	MeshVPN* app;
	int Socket;
	std::vector<RouteServer4Client*> Clients;
	Switch* Parent;
public:
	RouteServer4(MeshVPN* app, Switch* Parent, struct in_addr Address, unsigned short Port);
	~RouteServer4();
	int GetHandle();
	bool ReadHandle();
	void DeleteClient(RouteServer4Client* Client);
	void AdvertiseRoute(struct in_addr* Destination, int PrefixLength, int Metric);
	void WithdrawRoute(struct in_addr* Destination, int PrefixLength, int Metric);
};