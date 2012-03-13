// accept.c

#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_accept   (Task* task,  Val arg)   {
    //=================
    //
    // Mythryl type:   Socket -> (Socket, Address)
    //
    // This fn gets bound as   accept'   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Sock_accept");

    int		socket = TAGGED_INT_TO_C_INT( arg );				// Last use of 'arg'.
    char	address_buf[  MAX_SOCK_ADDR_BYTESIZE ];
    socklen_t	address_len = MAX_SOCK_ADDR_BYTESIZE;
    int		new_socket;

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_accept", NULL );
	//
    /*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c	*/

	    new_socket = accept (socket, (struct sockaddr*) address_buf, &address_len);

    /*  } while (new_socket < 0 && errno == EINTR);	*/		/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_accept" );

    if (new_socket == -1) {
        //
	return  RAISE_SYSERR__MAY_HEAPCLEAN( task, new_socket, NULL);
        //
    } else {
        //
	Val data    =  make_biwordslots_vector_sized_in_bytes__may_heapclean(	task, address_buf,                  address_len, NULL );
	Val address =  make_vector_header(					task, UNT8_RO_VECTOR_TAGWORD, data, address_len);

	return  make_two_slot_record(task,  TAGGED_INT_FROM_C_INT( new_socket ), address);
    }
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

