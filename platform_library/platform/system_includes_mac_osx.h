#include <new>
#include "stdio.h"
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>

#include <sys/ioctl.h>   /* ioctl() */
#define NO_IPX_SUPPORT
typedef sockaddr_in SOCKADDR_IN;
typedef sockaddr * PSOCKADDR;
typedef sockaddr SOCKADDR;
typedef in_addr IN_ADDR;
typedef int SOCKET;

#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1

#define closesocket close
