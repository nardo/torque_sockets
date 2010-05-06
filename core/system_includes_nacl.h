#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>

#define XP_UNIX

typedef __uint8_t		sa_family_t;
struct  hostent {
	char    *h_name;        /* official name of host */
	char    **h_aliases;    /* alias list */
	int     h_addrtype;     /* host address type */
	int     h_length;       /* length of address */
	char    **h_addr_list;  /* list of addresses from name server */
#define h_addr  h_addr_list[0]  /* address, for backward compatiblity */
};
struct hostent	*gethostbyname(const char *);

struct sockaddr {
	unsigned char sa_len;
	unsigned char sa_family;
	char sa_data[14];
};
typedef __uint32_t      in_addr_t;      /* base type for internet address */
 typedef __uint16_t      in_port_t;
 struct in_addr {
 in_addr_t s_addr;
 };
 
struct sockaddr_in {
	__uint8_t sin_len;
	__uint8_t sin_family;
	in_port_t sin_port;
	struct in_addr sin_addr;
	char sin_zero[8];
};

in_addr_t inet_addr(const char *);
typedef	unsigned int socklen_t;

#define	SOCK_DGRAM 2
#define	AF_INET 2
#define	SOL_SOCKET 0xffff
#define SO_SNDBUF 0x1001
#define SO_RCVBUF 0x1002
#define FIONBIO 1
#define	SO_BROADCAST 0x0020
#define	INADDR_ANY 0x00000000
#define	INADDR_BROADCAST 0xffffffff
#define	INADDR_LOOPBACK 0x7f000001
#define	INADDR_NONE 0xffffffff

int	ioctl(int, unsigned long, ...) { return 0; }
int bind(int, const struct sockaddr *, socklen_t) { return 0; }
int getsockname(int, struct sockaddr * __restrict, socklen_t * __restrict) { return 0; }
int	getsockopt(int, int, int, void * __restrict, socklen_t * __restrict) { return 0; }
int	setsockopt(int, int, int, const void *, socklen_t) { return 0; }
ssize_t	recvfrom(int, void *, size_t, int, struct sockaddr * __restrict, socklen_t * __restrict) { return 0; }
ssize_t	sendto(int, const void *, size_t, int, const struct sockaddr *, socklen_t) { return 0; }
int	socket(int, int, int) { return 0; }
__uint32_t	htonl(__uint32_t) { return 0; }
__uint16_t	htons(__uint16_t) { return 0; }
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




/*
 * Option flags per-socket.
 */
#define	SO_DEBUG	0x0001		/* turn on debugging info recording */
#define	SO_ACCEPTCONN	0x0002		/* socket has had listen() */
#define	SO_REUSEADDR	0x0004		/* allow local address reuse */
#define	SO_KEEPALIVE	0x0008		/* keep connections alive */
#define	SO_DONTROUTE	0x0010		/* just use interface addresses */
#define	SO_BROADCAST	0x0020		/* permit sending of broadcast msgs */
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define	SO_USELOOPBACK	0x0040		/* bypass hardware when possible */
#define SO_LINGER	0x0080          /* linger on close if data present (in ticks) */
#else
#define SO_LINGER	0x1080          /* linger on close if data present (in seconds) */
#endif	/* (!_POSIX_C_SOURCE || _DARWIN_C_SOURCE) */
#define	SO_OOBINLINE	0x0100		/* leave received OOB data in line */
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define	SO_REUSEPORT	0x0200		/* allow local address & port reuse */
#define	SO_TIMESTAMP	0x0400		/* timestamp received dgram traffic */
#ifndef __APPLE__
#define	SO_ACCEPTFILTER	0x1000		/* there is an accept filter */
#else
#define SO_DONTTRUNC	0x2000		/* APPLE: Retain unread data */
/*  (ATOMIC proto) */
#define SO_WANTMORE		0x4000		/* APPLE: Give hint when more data ready */
#define SO_WANTOOBFLAG	0x8000		/* APPLE: Want OOB in MSG_FLAG on receive */
#endif
#endif	/* (!_POSIX_C_SOURCE || _DARWIN_C_SOURCE) */

/*
 * Additional options, not kept in so_options.
 */
#define SO_SNDLOWAT	0x1003		/* send low-water mark */
#define SO_RCVLOWAT	0x1004		/* receive low-water mark */
#define SO_SNDTIMEO	0x1005		/* send timeout */
#define SO_RCVTIMEO	0x1006		/* receive timeout */
#define	SO_ERROR	0x1007		/* get error status and clear */
#define	SO_TYPE		0x1008		/* get socket type */
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
/*efine	SO_PRIVSTATE	0x1009		   get/deny privileged state */
#ifdef __APPLE__
#define SO_NREAD	0x1020		/* APPLE: get 1st-packet byte count */
#define SO_NKE		0x1021		/* APPLE: Install socket-level NKE */
#define SO_NOSIGPIPE	0x1022		/* APPLE: No SIGPIPE on EPIPE */
#define SO_NOADDRERR	0x1023		/* APPLE: Returns EADDRNOTAVAIL when src is not available anymore */
#define SO_NWRITE	0x1024		/* APPLE: Get number of bytes currently in send socket buffer */
#define SO_REUSESHAREUID	0x1025		/* APPLE: Allow reuse of port/socket by different userids */
#ifdef __APPLE_API_PRIVATE
#define SO_NOTIFYCONFLICT	0x1026	/* APPLE: send notification if there is a bind on a port which is already in use */
#endif
#define SO_LINGER_SEC	0x1080          /* linger on close if data present (in seconds) */
#define SO_RESTRICTIONS	0x1081	/* APPLE: deny inbound/outbound/both/flag set */
#define SO_RESTRICT_DENYIN		0x00000001	/* flag for SO_RESTRICTIONS - deny inbound */
#define SO_RESTRICT_DENYOUT		0x00000002	/* flag for SO_RESTRICTIONS - deny outbound */
#define SO_RESTRICT_DENYSET		0x80000000	/* flag for SO_RESTRICTIONS - deny has been set */
#endif
#define	SO_LABEL	0x1010		/* socket's MAC label */
#define	SO_PEERLABEL	0x1011		/* socket's peer MAC label */
#endif	/* (!_POSIX_C_SOURCE || _DARWIN_C_SOURCE) */

