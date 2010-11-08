/* socket.c
 *
 */

#include "../../config.h"

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#include "print-if.h"

/*
###       "Transmission of documents via telephone wires
###        is possible in principle, but the apparatus
###        required is so expensive that it will never
###        become a practical proposition."
###
###                           -- Dennis Gabor, 1962
###                              British physicist,
###                              author of Inventing the Future. 
 */



static char* domain_name( int domain ) {

    switch (domain) {

	#ifdef AF_UNIX				/* AF_LOCAL has same numeric value as AF_UNIX, so we skip it here.	*/
	case   AF_UNIX:  return "AF_UNIX";
	#endif

	#ifdef AF_INET
	case   AF_INET:  return "AF_INET";
	#endif

	#ifdef AF_INET6
	case   AF_INET6:  return "AF_INET6";
	#endif

	#ifdef AF_IPIX
	case   AF_IPX:  return "AF_IPX";			/* Novell						*/
	#endif

	#ifdef AF_NETLINK
	case   AF_NETLINK:  return "AF_NETLINK";		/* kernel user device -- see netlink(7) manpage. 	*/
	#endif

	#ifdef AF_X25
	case   AF_X25:  return "AF_X25";			/* ITU-T X.25 / ISO-8208 protocol   x25(7) manpage 	*/
	#endif

	#ifdef AF_AX25
	case   AF_AX25:  return "AF_AX25";			/* Amateur radio AX.25 protocol				*/
	#endif

	#ifdef AF_ATMPVC
	case   AF_ATMPVC:  return "AF_ATMPVC";		/* Access to raw ATM PVCs				*/
	#endif

	#ifdef AF_APPLETALK
	case   AF_APPLETALK:  return "AF_APPLETALK";	/* See ddp(7) manpage.					*/
	#endif

	#ifdef AF_PACKET
	case   AF_PACKET:  return "AF_PACKET";		/* Low level packet interface       packet(7) manpage	*/
	#endif

	default:	return "Unknown protocol family";
    }
}

static char* type_name( int type ) {

    static char buf[128];
    int    t = type;

    /* Since Linux 2.6.27, the following two flags
     * may be ORed into the type
     */
    #ifdef SOCK_NONBLOCK	/* Used to save making the equivalent fcntl call -- see fcntl(2) manpage.	*/
    #ifdef SOCK_CLOEXEC		/* close-on-exec -- see open(2) manpage.					*/

    t = t & ~(SOCK_NONBLOCK | SOCK_CLOEXEC);

    #endif
    #endif

    switch (t) {

	#ifdef SOCK_STREAM
	case   SOCK_STREAM:  strcpy(buf, "SOCK_STREAM");	break;
	#endif

	#ifdef SOCK_DGRAM
	case   SOCK_DGRAM:   strcpy(buf, "SOCK_DGRAM");		break;
	#endif

	#ifdef SOCK_SEQPACKET
	case   SOCK_SEQPACKET:  strcpy(buf, "SOCK_SEQPACKET");	break;
	#endif

	#ifdef SOCK_RAW
	case   SOCK_RAW:  strcpy(buf, "SOCK_RAW");		break;
	#endif

	#ifdef SOCK_RDM
	case   SOCK_RDM:  strcpy(buf, "SOCK_RDM");		break;		/* Reliable datagrams (no order-of-delivery guarantee).	*/
	#endif

	#ifdef SOCK_PACKET
	case   SOCK_PACKET:  strcpy(buf, "SOCK_PACKET");	break;		/* Obsolete -- see packet(7) manpage.			*/
	#endif

	default:	strcpy(buf, "Unknown socket type");	break;
    }

    #ifdef SOCK_NONBLOCK
	if (type & SOCK_NONBLOCK) {
	    strcat(buf, "|SOCK_NONBLOCK");
	}
    #endif

    #ifdef SOCK_CLOEXEC
	if (type & SOCK_CLOEXEC) {
	    strcat(buf, "|SOCK_CLOEXEC");
	}
    #endif

    return buf;
}

/* _lib7_Sock_socket : (Int, Int, Int) -> Socket_Fd
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/generic-socket.pkg
 */
lib7_val_t _lib7_Sock_socket (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int domain   =  REC_SELINT(arg, 0);
    int type     =  REC_SELINT(arg, 1);
    int protocol =  REC_SELINT(arg, 2);

    int sock;

    print_if( "socket.c/top: domain d=%d (%s) type d=%d (%s) protocol d=%d\n", domain, domain_name(domain), type, type_name(type), protocol );
    errno = 0;

    sock     = socket (domain, type, protocol);

    print_if( "socket.c/bot: socket d=%d errno d=%d\n", sock, errno );


    if (sock < 0)   return RAISE_SYSERR(lib7_state, status);	/* RAISE_SYSERR is defined in src/runtime/c-libs/lib7-c.h */
    else	    return INT_CtoLib7(sock);
}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
