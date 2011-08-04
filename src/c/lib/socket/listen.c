// listen.c


#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
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

    int socket  =  GET_TUPLE_SLOT_AS_INT( arg, 0 );
    int backlog =  GET_TUPLE_SLOT_AS_INT( arg, 1 );

    int status =  listen( socket, backlog );

    CHECK_RETURN_UNIT( task, status );
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

