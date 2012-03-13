// get-or-set-socket-linger-option.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "socket-util.h"
#include "cfun-proto-list.h"

/*
###        "We hope the Professor from Clark College
###         (Robert H. Goddard) is only pretending
###         to be ignorant of elementary physics
###         if he thinks that a rocket can work in a vacuum."
###
###                 -- Editorial, The New York Times 1920
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   get_or_set_socket_linger_option   (Task* task,  Val arg)   {
    //===============================
    //
    // Mythryl type: (Socket_Fd, Null_Or(Null_Or(Int))) -> Null_Or(Int)
    //
    // Set/get the SO_LINGER option as follows:
    //   NULL		=> get current setting
    //   THE(NULL)	=> disable linger
    //   THE(THE t)	=> enable linger with timeout t.
    //
    // This function gets bound as   ctl_linger   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg
    //

													ENTER_MYTHRYL_CALLABLE_C_FN("get_or_set_socket_linger_option");

    int  socket = GET_TUPLE_SLOT_AS_INT( arg, 0 );
    Val	    ctl = GET_TUPLE_SLOT_AS_VAL( arg, 1 );							// Last use of 'arg'.

    struct linger   optVal;
    int		    status;

    if (ctl == OPTION_NULL) {
        //
	socklen_t  optSz =  sizeof( struct linger );

	RELEASE_MYTHRYL_HEAP( task->pthread, "get_or_set_socket_linger_option", NULL );
	    //
	    status =  getsockopt( socket, SOL_SOCKET, SO_LINGER, (sockoptval_t)&optVal, &optSz );
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "get_or_set_socket_linger_option" );

	ASSERT( status < 0  ||  optSz == sizeof( struct linger ));
	//
    } else {
	//
	ctl = OPTION_GET(ctl);

	if (ctl == OPTION_NULL) {
	    optVal.l_onoff = 0;	    // Argument is THE(NULL); disable linger.
	} else {
	    optVal.l_onoff = 1;	    // argument is THE t; enable linger.
	    optVal.l_linger = TAGGED_INT_TO_C_INT(OPTION_GET(ctl));
	}

	RELEASE_MYTHRYL_HEAP( task->pthread, "get_or_set_socket_linger_option", NULL );
	    //
	    status = setsockopt (socket, SOL_SOCKET, SO_LINGER, (sockoptval_t)&optVal, sizeof(struct linger));
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "get_or_set_socket_linger_option" );
    }

    if (status < 0)  		return RAISE_SYSERR__MAY_HEAPCLEAN(task, status, NULL);
    if (optVal.l_onoff == 0)    return OPTION_NULL;

    return   OPTION_THE(  task,  TAGGED_INT_FROM_C_INT( optVal.l_linger )  );
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

