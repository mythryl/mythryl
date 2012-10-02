// listen.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

/*
###      "A good listener is not only popular everywhere,
###       but after a while, knows something."
###
###                       -- Wilson Mizner
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_listen   (Task* task,  Val arg)   {
    //=================
    //
    // Mythryl type: (Socket, Int) -> Void
    //
    // This fn gets bound as   listen'   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

										ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Sock_listen");

    int socket  =  GET_TUPLE_SLOT_AS_INT( arg, 0 );
    int backlog =  GET_TUPLE_SLOT_AS_INT( arg, 1 );				// Last use of 'arg'.

    RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_Sock_listen", NULL );
	//
	int status =  listen( socket, backlog );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_Sock_listen" );

    return  RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN( task, status, NULL );
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

