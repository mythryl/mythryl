// socket.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <sys/types.h>          // See NOTES
#include <sys/socket.h>
#include <string.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "raise-error.h"
#include "cfun-proto-list.h"


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



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



static char*   domain_name   (int domain) {
    //         ===========
    //
    switch (domain) {

	#ifdef AF_UNIX						// AF_LOCAL has same numeric value as AF_UNIX, so we skip it here.
	case   AF_UNIX:  return "AF_UNIX";
	#endif

	#ifdef AF_INET
	case   AF_INET:  return "AF_INET";
	#endif

	#ifdef AF_INET6
	case   AF_INET6:  return "AF_INET6";
	#endif

	#ifdef AF_IPIX
	case   AF_IPX:  return "AF_IPX";			// Novell
	#endif

	#ifdef AF_NETLINK
	case   AF_NETLINK:  return "AF_NETLINK";		// kernel user device -- see netlink(7) manpage.
	#endif

	#ifdef AF_X25
	case   AF_X25:  return "AF_X25";			// ITU-T X.25 / ISO-8208 protocol   x25(7) manpage
	#endif

	#ifdef AF_AX25
	case   AF_AX25:  return "AF_AX25";			// Amateur radio AX.25 protocol
	#endif

	#ifdef AF_ATMPVC
	case   AF_ATMPVC:  return "AF_ATMPVC";			// Access to raw ATM PVCs
	#endif

	#ifdef AF_APPLETALK
	case   AF_APPLETALK:  return "AF_APPLETALK";		// See ddp(7) manpage.
	#endif

	#ifdef AF_PACKET
	case   AF_PACKET:  return "AF_PACKET";			// Low level packet interface       packet(7) manpage
	#endif

	default:	return "Unknown protocol family";
    }
}

static char*   name_of_type   (int type)    {
    //         =========
    //
    static char buf[128];
    int    t = type;

    // Since Linux 2.6.27, the following two flags
    // may be ORed into the type
    //
    #ifdef SOCK_NONBLOCK	// Used to save making the equivalent fcntl call -- see fcntl(2) manpage.
    #ifdef SOCK_CLOEXEC		// close-on-exec -- see open(2) manpage.
	//
        t = t & ~(SOCK_NONBLOCK | SOCK_CLOEXEC);
	//
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
	case   SOCK_RDM:  strcpy(buf, "SOCK_RDM");		break;		// Reliable datagrams (no order-of-delivery guarantee).
	#endif

	#ifdef SOCK_PACKET
	case   SOCK_PACKET:  strcpy(buf, "SOCK_PACKET");	break;		// Obsolete -- see packet(7) manpage.
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



Val   _lib7_Sock_socket   (Task* task,  Val arg)   {
    //=================
    //
    // Mythryl type:  (Int, Int, Int) -> Socket_Fd				// (domain, type, protocol) -> Socket_Fd
    //
    // This fn gets bound to   c_socket   in:
    //
    //     src/lib/std/src/socket/plain-socket--premicrothread.pkg

										ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int domain   =  GET_TUPLE_SLOT_AS_INT( arg, 0 );
    int type     =  GET_TUPLE_SLOT_AS_INT( arg, 1 );
    int protocol =  GET_TUPLE_SLOT_AS_INT( arg, 2 );				// Last use of 'arg'.

										if (0)	log_if( "socket.c/top: domain d=%d (%s) type d=%d (%s) protocol d=%d\n", domain, domain_name(domain), type, name_of_type(type), protocol );
    errno = 0;

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	int sock =  socket (domain, type, protocol);				// socket	documented in   man 2 socket
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );
//										log_if( "socket.c/bot: socket d=%d errno d=%d\n", sock, errno );
    Val result;

    if (sock < 0)   result =  RAISE_SYSERR__MAY_HEAPCLEAN(task, status, NULL);	// RAISE_SYSERR__MAY_HEAPCLEAN is defined in src/c/lib/raise-error.h
    else	    result =  TAGGED_INT_FROM_C_INT( sock );
										// 'status' looks bogus here (ignored except on MacOS). XXX BUGGO FIXME

										EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

