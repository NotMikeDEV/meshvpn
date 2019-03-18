#pragma once
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <asm/types.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/epoll.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#define NODE_KEY_LENGTH 10
#define ROUTING_PROTOCOL 33

#include "Crypto.h"
#include "SocketHandler.h"
#include "RemoteNode.h"
#include "Switch.h"
#include "TunTap.h"
#include "MeshVPN.h"
#include "RouteServer4.h"
#include "RouteClient4.h"
#include "RouteServer6.h"
#include "RouteClient6.h"

#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

extern unsigned char Debug;
