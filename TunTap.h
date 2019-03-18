class Switch;
class TunTap : public SocketHandler {
private:
	struct ifreq ifr;
	int fd;
	Switch* LocalSwitch;
	struct in_addr IPv4;
	struct in6_addr IPv6;
public:
	TunTap(char* InterfaceName, Switch* Switch);
	int GetHandle();
	bool ReadHandle();
	void SetIPv4(struct in_addr* IP, int PrefixLength);
	void SetIPv6(struct in6_addr* IP, int PrefixLength);
	struct in_addr* GetIPv4();
	struct in6_addr* GetIPv6();
	void WritePacket(unsigned char* Packet, int Length);
};
