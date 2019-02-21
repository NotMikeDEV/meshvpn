unsigned char node_key[6];
struct IPv4_router_conn{
	unsigned char State;
	int Socket;
	unsigned char* RecvBuffer[255];
	unsigned char RecvOffset;
	int Latency;
	struct RouteEntry4* RouteList;
};
struct IPv6_router_conn{
	unsigned char State;
	int Socket;
	unsigned char* RecvBuffer[255];
	unsigned char RecvOffset;
	int Latency;
	struct RouteEntry6* RouteList;
};
struct Node{
    unsigned char Static;
    unsigned char State;
    unsigned char Retries;
    struct sockaddr_in6 address;
    unsigned char Key[sizeof(node_key)];
    time_t LastRecv;
    time_t LastSend;
    time_t LastSendNodeList;
    struct in_addr IPv4;
    struct IPv4_router_conn RouterConn4;
    struct in6_addr IPv6;
    struct IPv6_router_conn RouterConn6;
    struct Node* Next;
    struct Node* Previous;
    struct timeval LastPing;
    int Latency;
};
struct Node* FirstNode;
struct EthernetNode{
    unsigned char MAC[6];
    struct Node* Node;
    struct EthernetNode* Next;
    struct EthernetNode* Previous;
};
struct EthernetNode* FirstEthernetNode;
struct Node* node_add(struct sockaddr_in6* addr);
void node_delete(struct Node* node);
struct Node* node_find_addr(struct sockaddr_in6* addr);
struct Node* node_find_key(unsigned char* key);
struct EthernetNode* node_add_mac(struct Node* node, unsigned char* MAC);
void node_delete_macs(struct Node* node);
struct Node* node_find_mac(unsigned char* MAC);

