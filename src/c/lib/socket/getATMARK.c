// getATMARK.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

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



Val   _lib7_Sock_getATMARK   (Task* task,  Val arg)   {
    //====================
    //
    // Mythryl type:   Socket_Fd -> Int
    //
    // This fn gets bound as   get_atmark'   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg
    //

											ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int device = TAGGED_INT_TO_C_INT( arg );						// Last use of 'arg'.

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	int	                                         n;
	int status = ioctl (device, SIOCATMARK, (char*) &n );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    if (status < 0)     return RAISE_SYSERR__MAY_HEAPCLEAN(task, status, NULL);

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return    n ? HEAP_TRUE : HEAP_FALSE;
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.

