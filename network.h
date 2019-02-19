void init_network(uint16_t UDPPort);
void network_parse(unsigned char* packet, int length, struct sockaddr_in6* addr);
void network_send_ethernet_packet(unsigned char* buffer, int len);
void network_send_pings();
void network_send_nodelists();
void network_send_init_packets();
void network_purge_nodelist();
struct in_addr node_IPv4;
short node_IPv4_pl;
struct in6_addr node_IPv6;
short node_IPv6_pl;

#define NODE_LIST_MAX 50
struct NodeEntry{
    unsigned char address[16];
    unsigned short port;
};
