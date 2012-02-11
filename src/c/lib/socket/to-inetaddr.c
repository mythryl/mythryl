// to-inetaddr.c


#include "../../mythryl-config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



/*
###               "The Americans have need of the telephone, but
###                we do not. We have plenty of messenger boys."
###
###                         --- Sir William Preece, 1878
###                             Chief Engineer, British Post Office
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_toinetaddr   (Task* task,  Val arg)   {
    //=====================
    //	
    // Mythryl type:   (Internet_Address, Int) -> Internet_Address
    //
    // Given a INET address and port number, allocate a INET-domain socket address.
    //
    // This fn gets bound as   to_inet_addr   in:
    //
    //     src/lib/std/src/socket/internet-socket.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Sock_toinetaddr");

    Val	      inAddr =  GET_TUPLE_SLOT_AS_VAL( arg, 0 );
    uint16_t  port   =  GET_TUPLE_SLOT_AS_INT( arg, 1 );	// Port in host byte order.

    struct sockaddr_in	addr;
    memset(            &addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;

    memcpy (
	&addr.sin_addr,
	GET_VECTOR_DATACHUNK_AS( char*, inAddr ),
	sizeof(struct in_addr));

    addr.sin_port = htons(port);			// port in network byte order.

    Val data =  make_biwordslots_vector_sized_in_bytes( task, &addr, sizeof(struct sockaddr_in) );

    return  make_vector_header(task,  UNT8_RO_VECTOR_TAGWORD, data, sizeof(struct sockaddr_in));
}




// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

