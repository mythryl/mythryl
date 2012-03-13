// inetany.c


#include "../../mythryl-config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"



/*
###           "Radio has no future."
###                   -- Lord Kelvin, 1897
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_inetany   (Task* task,  Val arg)   {
    //==================
    //
    // Mythryl type:   Int -> Internet_Address
    //
    // Make an INET_ANY INET socket address, with the given port ID.
    //
    // This fn gets bound as   inet_any   in:
    //
    //     src/lib/std/src/socket/internet-socket.pkg
    //

																ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Sock_inetany");

    struct sockaddr_in	addr;
    memset(            &addr, 0, sizeof(struct sockaddr_in) );

    addr.sin_family      =  AF_INET;
    addr.sin_addr.s_addr =  htonl( INADDR_ANY );
    addr.sin_port        =  htons( TAGGED_INT_TO_C_INT( arg ) );								// Last use of 'arg'.

    Val data =  make_biwordslots_vector_sized_in_bytes__may_heapclean(	task, &addr, sizeof(struct sockaddr_in), NULL );

    return make_vector_header(task,  UNT8_RO_VECTOR_TAGWORD, data, sizeof(struct sockaddr_in) );
}



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

