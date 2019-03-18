#include "NetLink6.h"
#include "RouteServer6Client.h"

class RouteServer6 :public SocketHandler {
private:
	MeshVPN* app;
	int Socket;
	std::vector<RouteServer6Client*> Clients;
	Switch* Parent;
public:
	RouteServer6(MeshVPN* app, Switch* Parent, struct in6_addr Address, unsigned short Port);
	~RouteServer6();
	int GetHandle();
	bool ReadHandle();
	void DeleteClient(RouteServer6Client* Client);
	void AdvertiseRoute(struct in6_addr* Destination, int PrefixLength, int Metric);
	void WithdrawRoute(struct in6_addr* Destination, int PrefixLength, int Metric);
};