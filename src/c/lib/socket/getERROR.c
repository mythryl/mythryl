// getERROR.c

#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_getERROR   (Task* task,  Val arg)   {
    //===================
    //
    // Mythryl type:   Socket_Fd -> Bool
    //
    // This fn gets bound as   get_error'   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

													ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int	socket =  TAGGED_INT_TO_C_INT( arg );								// Last use of 'arg'.

    socklen_t	opt_size = sizeof(int);

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	int	                                                                flag;
	int status =  getsockopt( socket, SOL_SOCKET, SO_ERROR, (sockoptval_t) &flag, &opt_size );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    if (status < 0)     return RAISE_SYSERR__MAY_HEAPCLEAN(task, status, NULL);

													EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return   flag ? HEAP_TRUE : HEAP_FALSE;
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

