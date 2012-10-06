// getTYPE.c

#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"
#include "socket-util.h"



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_getTYPE   (Task* task,  Val arg)   {		//  : Socket -> Sock_type
    //==================
    //
    // Mythryl type:   Socket_Fd -> ci::System_Constant
    //
    // This fn gets bound as   get_type'   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

													ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int socket = TAGGED_INT_TO_C_INT( arg );								// Last use of 'arg'.

    socklen_t opt_size = sizeof( int );

    RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_Sock_getTYPE", NULL );
	//
	int flag;
	int status =  getsockopt( socket, SOL_SOCKET, SO_TYPE, (sockoptval_t)&flag, &opt_size );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_Sock_getTYPE" );

    if (status < 0)     return RAISE_SYSERR__MAY_HEAPCLEAN(task, status, NULL);
    else		return make_system_constant__may_heapclean( task, &_Sock_Type, flag, NULL );
}



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

