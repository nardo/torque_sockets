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
#include <ifaddrs.h>
#include <mach/mach_vm.h>
#include <mach/vm_map.h>
#include <mach/mach_init.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stddef.h>

#include <CoreServices/CoreServices.h>

#include <sys/ioctl.h>   /* ioctl() */
#define NO_IPX_SUPPORT
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr * PSOCKADDR;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;
typedef int SOCKET;

#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1

#define closesocket close

#ifdef __cplusplus
#include <new>
#include <queue>
#endif
