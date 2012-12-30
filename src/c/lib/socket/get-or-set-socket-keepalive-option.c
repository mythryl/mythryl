// get-or-set-socket-keepalive-option.c


#include "../../mythryl-config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "raise-error.h"
#include "socket-util.h"
#include "cfun-proto-list.h"

/*
###           "Aerial flight is one of that
###            class of problems with which man
###            will never be able to cope."
###
###                       -- Simon Newcomb, 1920
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   get_or_set_socket_keepalive_option   (Task* task,  Val arg)   {
    //==================================
    //
    // Mythryl type:  (Socket_Fd, Null_Or(Bool)) -> Bool
    //
    // This function gets bound as   ctl_keepalive   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg
												ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Val result =  get_or_set_boolean_socket_option( task, arg, SO_KEEPALIVE );			// get_or_set_boolean_socket_option		def in    src/c/lib/socket/get-or-set-boolean-socket-option.c
	//
	// We do the RELEASE_MYTHRYL_HEAP/RECOVER_MYTHRYL_HEAP stuff in
	// get_or_set_boolean_socket_option().

												EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

