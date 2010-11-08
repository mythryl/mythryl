/* sockets-osdep.h
 *
 *
 * O.S. specific dependencies needed by the sockets library.
 */

#ifndef _SOCKETS_OSDEP_
#define _SOCKETS_OSDEP_

#include "../../config.h"

#if defined(OPSYS_UNIX)
#  define HAS_UNIX_DOMAIN
/* Should probably change this to use HAVE_SYS_SOCKET_H, but watch out for the windows version below:  XXX BUGGO FIXME */
#  define INCLUDE_SOCKET_H	<sys/socket.h>
#  define INCLUDE_IN_H		<netinet/in.h>
#  define INCLUDE_TCP_H		<netinet/tcp.h>
#  define INCLUDE_UN_H		<sys/un.h>

#  if defined(OPSYS_SOLARIS)
#    define INCLUDE_RPCENT_H	<rpc/rpcent.h>

typedef char *sockoptval_t;	/* The pointer type used to pass values to */
				/* getsockopt/setsockopt */

#    define BSD_COMP		/* needed to include FION* in ioctl.h */

#  else
typedef void *sockoptval_t;	/* The pointer type used to pass values to */
				/* getsockopt/setsockopt */
#  endif

#  if (defined(OPSYS_AIX))
#    define _SUN		/* to get the rpcent definitions */
#    define SOCKADDR_HAS_LEN	/* socket address has a length field */
#  endif

#  if (defined(OPSYS_FREEBSD) || defined (OPSYS_NETBSD) || defined (OPSYS_NETBSD2))
#    define i386		1	/* to avoid a bug in system header files */
#    define INCLUDE_RPCENT_H	<rpc/rpc.h>
#  endif

#include "runtime-unixdep.h"
/* FIXME: The following includes are not needed in every file, yet they
   cannot be moved to where they are used since that would break compilation
   under Windows */

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include INCLUDE_IN_H
#include INCLUDE_TCP_H
#include <netdb.h>
#include <sys/ioctl.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#elif defined(OPSYS_WIN32) || defined(OPSYS_CYGWIN)
#  define INCLUDE_SOCKET_H      <winsock2.h>

/* FIXME:  Is ioctlsocket() on Windows really the same as ioctl() on Unix?
   It does seem so, yet the second parameter is of a different type */
#  define ioctl ioctlsocket

typedef char *sockoptval_t;	/* The pointer type used to pass values to */
				/* getsockopt/setsockopt */
#endif

#define MAX_SOCK_ADDR_SZB	1024

#endif /* !_SOCKETS_OSDEP_ */



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
