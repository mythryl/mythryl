// from-inetaddr.c


#include "../../mythryl-config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"


/*
###          "The first 90% of the code accounts for
###           the first 90% of the development time.
###           The remaining 10% of the code accounts for
###           the other 90% of the development time."
###
###                            -- Tom Cargill
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_frominetaddr   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:   Internet_Address -> (Internet_Address, Int)
    //
    // Given a INET-domain socket address, return the INET address and port number.
    //
    // This fn gets bound as   from_inet_addr   in:
    //
    //     src/lib/std/src/socket/internet-socket.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Sock_frominetaddr");

    struct sockaddr_in*	addr
        =
        GET_VECTOR_DATACHUNK_AS( struct sockaddr_in*, arg );

    Val data;
    Val inAddr;

    ASSERT( addr->sin_family == AF_INET );

    data   =  make_int2_vector_sized_in_bytes( task, &(addr->sin_addr), sizeof(struct in_addr) );
    inAddr =  make_vector_header( task,  UNT8_RO_VECTOR_TAGWORD, data,  sizeof(struct in_addr) );

    return  make_two_slot_record(task, inAddr, TAGGED_INT_FROM_C_INT(ntohs(addr->sin_port)) );
}



// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

