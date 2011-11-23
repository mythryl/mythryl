// socketpair.c
//
// NOTE: this file is UNIX specific.


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"
#include "sockets-osdep.h"

#include <stdio.h>
#include <string.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



/*
###                  "Computers in the future may weigh no more than 1.5 tons."
###
###                                            -- Popular Mechanics, 1949
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_socketpair   (Task* task,  Val arg)   {
    //=====================
    //
    // Mytyryl type: (Int, Int, Int) -> (Socket, Socket)
    //
    // Create a pair of sockets.
    //
    // The arguments are:
    //
    //     domain (should be AF_UNIX)
    //     type
    //     protocol.
    //
    // This fn gets bound to   c_socket_pair   in
    //
    //     src/lib/std/src/socket/plain-socket.pkg

    int	 domain   =  GET_TUPLE_SLOT_AS_INT( arg, 0 );
    int	 type     =  GET_TUPLE_SLOT_AS_INT( arg, 1 );
    int	 protocol =  GET_TUPLE_SLOT_AS_INT( arg, 2 );

    int	 socket[2];

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_socketpair", arg );
	//
	int status
	    =
	    socketpair(					// socketpair	documented in   man 2 socketpair
		//
		domain,
		type,
		protocol,
		socket
	    );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_socketpair" );

    if (status < 0)   return RAISE_SYSERR(task, status);

    Val	             result;
    REC_ALLOC2(task, result, TAGGED_INT_FROM_C_INT(socket[0]), TAGGED_INT_FROM_C_INT(socket[1]));
    return           result;
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


