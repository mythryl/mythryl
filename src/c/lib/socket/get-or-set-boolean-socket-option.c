// get-or-set-boolean-socket-option.c


#include "../../mythryl-config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   get_or_set_boolean_socket_option   (Task* task,  Val arg,  int option)   {
    //================================
    //
    // Mythryl type:
    //
    // This utility routine gets/sets a boolean socket option.


    int	socket = GET_TUPLE_SLOT_AS_INT(arg, 0);
    Val	ctl    = GET_TUPLE_SLOT_AS_VAL(   arg, 1);

    int	flag, status;

    if (ctl == OPTION_NULL) {
        //
	socklen_t option_len = sizeof(int);
	status = getsockopt (socket, SOL_SOCKET, option, (sockoptval_t)&flag, &option_len);
	ASSERT((status < 0) || (option_len == sizeof(int)));

    } else {

	flag = TAGGED_INT_TO_C_INT(OPTION_GET(ctl));
	status = setsockopt (socket, SOL_SOCKET, option, (sockoptval_t)&flag, sizeof(int));
    }

    if (status < 0)	return  RAISE_SYSERR( task, status );
    else	        return  (flag ? HEAP_TRUE : HEAP_FALSE);
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

