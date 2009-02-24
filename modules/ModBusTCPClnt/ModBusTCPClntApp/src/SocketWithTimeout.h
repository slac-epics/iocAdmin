/* This is a small library to support TCP socket read and write with */
/* timeout. We don't use osiSock.h, but we just copy some header definition */
/* from it. The reason we do this is to support non-epics paltform */

/* For read, write, socket, select, they are standard for every OS */
/* We provide socket_close, socket_ioctl and xxxWithTimeout here */
#ifndef	_SocketWithTimeout_H_
#define	_SocketWithTimeout_H_

#ifdef _WIN32
#include <winsock2.h>
#pragma pack(push, 1)

#define SOCKERRNO       WSAGetLastError()
#define socket_close(S) closesocket(S)
#define socket_ioctl(A,B,C) ioctlsocket(A,B,C)
typedef u_long FAR osiSockIoctl_t;
#define SOCK_EWOULDBLOCK WSAEWOULDBLOCK
#define SOCK_EINPROGRESS WSAEINPROGRESS
#define SOCK_ETIMEDOUT   WSAETIMEDOUT
/* end of Win32 settings */

#else

#ifdef vxWorks
#include <vxWorks.h>
#include <sysLib.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sockLib.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <inetLib.h>
#include <ioLib.h>
#include <hostLib.h>
#include <selectLib.h>
#include <ctype.h>
#include <tickLib.h>
typedef int               SOCKET;
#define INVALID_SOCKET    (-1)
#define SOCKET_ERROR      (-1)
#define SOCKERRNO         errno
#define socket_close(S)   close(S)
#define socket_ioctl(A,B,C) ioctl(A,B,(int)C)
#define SOCK_EWOULDBLOCK  EWOULDBLOCK
#define SOCK_EINPROGRESS  EINPROGRESS
#define SOCK_ETIMEDOUT    ETIMEDOUT
/* end of vxWorks settings */

#else

/* Unix settings */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
typedef int               SOCKET;
#define INVALID_SOCKET    (-1)
#define SOCKET_ERROR      (-1)
#define SOCKERRNO         errno
#define socket_close(S)   close(S)
#define socket_ioctl(A,B,C) ioctl(A,B,C)
#define SOCK_EWOULDBLOCK  EWOULDBLOCK
#define SOCK_EINPROGRESS  EINPROGRESS
#define	SOCK_ETIMEDOUT    ETIMEDOUT

#ifdef SOLARIS
#include <sys/filio.h>
#define INADDR_NONE (-1)
#endif

/* end of Unix settings */
#endif
#endif

#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* vxWorks defined this funxtion, but Unix, RTEMS, and WIN32 don't. */
/* NOTE: base/src/RTEMS/base/rtems_util.c does define connectWithTimeout but this one should work too. */
/* Return codea ala vxWorks: OK == 0, ERROR == -1 */
#ifndef vxWorks
int connectWithTimeout (int sFd, const struct sockaddr *addr, int addr_size, struct timeval *ptimeout);
#endif

/* This function tries to read all of exact_size bytes until timeout for any read */
int readWithTimeout(int sFd, char *pbuf, size_t exact_size, struct timeval *ptimeout);

/* This function tries to send all of exact_size bytes until timeout for any write */
int writeWithTimeout(int sFd, const char *pbuf, size_t exact_size, struct timeval *ptimeout);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
