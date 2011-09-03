// getERROR.c

#include "../../config.h"

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



Val   _lib7_Sock_getERROR   (Task* task,  Val arg)   {
    //===================
    //
    // Mythryl type:   Socket_Fd -> Bool
    //
    // This fn gets bound as   get_error'   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

    int	socket =  TAGGED_INT_TO_C_INT( arg );

    socklen_t	opt_size = sizeof(int);

    int	                                                                    flag;
    int status =  getsockopt( socket, SOL_SOCKET, SO_ERROR, (sockoptval_t) &flag, &opt_size );

    if (status < 0)     return RAISE_SYSERR(task, status);

    return   flag ? HEAP_TRUE : HEAP_FALSE;
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

