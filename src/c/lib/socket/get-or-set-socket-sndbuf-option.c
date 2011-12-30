// get-or-set-socket-sndbuf-option.c

#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "socket-util.h"



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   get_or_set_socket_sndbuf_option   (Task* task,  Val arg)   {
    //===============================
    //
    // Mythryl type:   (Socket_Fd, Null_Or(Int)) -> Int
    //
    // This fn gets bound as   ctl_sndbuf   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

    ENTER_MYTHRYL_CALLABLE_C_FN("get_or_set_socket_sndbuf_option");

    int	socket =  GET_TUPLE_SLOT_AS_INT( arg, 0 );
    Val	ctl    =  GET_TUPLE_SLOT_AS_VAL( arg, 1 );

    int  size;
    int  status;

    if (ctl == OPTION_NULL) {
	//
	socklen_t opt_size = sizeof(int);

	RELEASE_MYTHRYL_HEAP( task->pthread, "get_or_set_socket_sndbuf_option", arg );
	    //
	    status = getsockopt (socket, SOL_SOCKET, SO_SNDBUF, (sockoptval_t)&size, &opt_size);
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "get_or_set_socket_sndbuf_option" );

	ASSERT((status < 0) || (opt_size == sizeof(int)));

    } else {

	size =  TAGGED_INT_TO_C_INT( OPTION_GET( ctl ) );

	RELEASE_MYTHRYL_HEAP( task->pthread, "get_or_set_socket_sndbuf_option", arg );
	    //
	    status =  setsockopt( socket, SOL_SOCKET, SO_SNDBUF, (sockoptval_t) &size, sizeof(int) );
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "get_or_set_socket_sndbuf_option" );
    }

    if (status < 0)     return  RAISE_SYSERR(task, status);

    return  TAGGED_INT_FROM_C_INT( size );
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

