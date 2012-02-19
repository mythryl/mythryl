// shutdown.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_shutdown   (Task* task,  Val arg)   {
    //===================
    //
    // Mythryl type: (Socket, Int) -> Void
    //
    // This fn gets bound to   shutdown'   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

													ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Sock_shutdown");

    int socket =  GET_TUPLE_SLOT_AS_INT( arg, 0 );
    int how    =  GET_TUPLE_SLOT_AS_INT( arg, 1 );							// RAISE_SYSERR__MAY_HEAPCLEAN	def in   src/c/lib/lib7-c.h
													// shutdown is documented by	man 2 shutdown
    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_shutdown", arg );
	//
	if (shutdown( socket, how ) < 0)   return  RAISE_SYSERR__MAY_HEAPCLEAN(task, status, NULL);	// Where is 'status' coming from? Is this rational?
	//												// ('status' is ignored except on MacOS, where this is probably broken) XXX BUGGO FIXME
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_shutdown" );

    return  HEAP_VOID;
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

