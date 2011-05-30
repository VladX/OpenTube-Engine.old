#ifndef SYS_SOCKET_H
#define SYS_SOCKET_H 1
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#define MSG_DONTWAIT 0
#undef EAGAIN
#define EAGAIN WSAEWOULDBLOCK
typedef unsigned int in_addr_t;
#endif
