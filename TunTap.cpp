#include "Globals.h"
TunTap::TunTap(char* InterfaceName, Switch* Switch)
{
	memset(&IPv4, 0, sizeof(&IPv4));
	memset(&IPv6, 0, sizeof(&IPv6));
	LocalSwitch = Switch;
	int err;

	if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
	{
		perror("tuntap not found");
		exit(-1);
	}

	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = IFF_TAP;
	if (*InterfaceName)
		strncpy(ifr.ifr_name, InterfaceName, IFNAMSIZ);

	if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0)
	{
		close(fd);
	}
	fcntl(fd, F_SETFL, O_NONBLOCK);
	int status;
	if (fork() == 0) execlp("ip", "ip", "link", "set", ifr.ifr_name, "up", NULL); else wait(&status);
	if (fork() == 0) execlp("ip", "ip", "link", "set", ifr.ifr_name, "mtu", "1280", NULL); else wait(&status);
	if (Debug)
		printf("Interface Name: %s\n", ifr.ifr_name);
}
void TunTap::SetIPv4(struct in_addr* IP, int PrefixLength)
{
	char IPv4[128];
	inet_ntop(AF_INET, IP, IPv4, sizeof(IPv4));
	sprintf(IPv4 + strlen(IPv4), "/%d", PrefixLength);
	int status;
	memcpy(&this->IPv4, IP, 4);
	if (fork() == 0) execlp("ip", "ip", "addr", "add", IPv4, "dev", ifr.ifr_name, NULL); else wait(&status);
	if (Debug)
		printf("IPv4: %s\n", IPv4);
}
void TunTap::SetIPv6(struct in6_addr* IP, int PrefixLength)
{
	char IPv6[128];
	inet_ntop(AF_INET6, IP, IPv6, sizeof(IPv6));
	sprintf(IPv6 + strlen(IPv6), "/%d", PrefixLength);
	int status;
	memcpy(&this->IPv6, IP, 16);
	if (fork() == 0) execlp("ip", "ip", "addr", "add", IPv6, "dev", ifr.ifr_name, NULL); else wait(&status);
	if (Debug)
		printf("IPv6: %s\n", IPv6);
}
struct in_addr* TunTap::GetIPv4()
{
	return &IPv4;
}
struct in6_addr* TunTap::GetIPv6()
{
	return &IPv6;
}
int TunTap::GetHandle()
{
	return fd;
}
bool TunTap::ReadHandle()
{
	unsigned char buffer[4096];
	int nread = read(fd, buffer, sizeof(buffer));
	if (nread > 0)
	{
		if (Debug > 2)
			printf("Read %d bytes from %s\n", nread, ifr.ifr_name);
		LocalSwitch->LocalEthernetPacket(buffer, nread);
		return true;
	}
	return false;
}
void TunTap::WritePacket(unsigned char* Packet, int Length)
{
	write(fd, Packet, Length);
	if (Debug > 2)
		printf("Write %d bytes to %s\n", Length, ifr.ifr_name);
}