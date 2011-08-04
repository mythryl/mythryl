// get-or-set-socket-reuseaddr-option.c


#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "socket-util.h"



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   get_or_set_socket_reuseaddr_option   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type: (Socket_Fd, Null_Or(Bool)) -> Bool
    //
    // This fn gets bound as   ctl_reuseaddr   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

    return   get_or_set_boolean_socket_option( task, arg, SO_REUSEADDR );			// get_or_set_boolean_socket_option		def in    src/c/lib/socket/get-or-set-boolean-socket-option.c
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

