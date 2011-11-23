// recvbuf.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/*
###                   "Before man reaches the moon, mail will be
###                    delivered within hours from New York to
###                    California, to Britain, to India or Australia
###                    by guided missiles.
###                       We stand on the threshold of rocket mail."
###
###                          -- Arthur Summerfield, US Postmaster General, 1959 
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_recvbuf   (Task* task,  Val arg)   {
    //==================
    //
    // Mythryl type:   (Socket, rw_vector_of_one_byte_unts::Rw_Vector, Int, Int, Bool, Bool) -> Int
    //
    // The arguments are:
    //     socket
    //     data buffer
    //     start position
    //     number of bytes,
    //     OOB flag
    //     peek flag.
    //
    // This fn gets bound as   recv_a   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

    int	  socket = GET_TUPLE_SLOT_AS_INT( arg, 0 );
//  Val	  buf    = GET_TUPLE_SLOT_AS_VAL( arg, 1 );	// Mythryl buffer in which to leave read bytes.		// We fetch this after the call, since the heapcleaner may move it during the call.
    int   offset = GET_TUPLE_SLOT_AS_INT( arg, 2 );	// Offset within buf for read bytes.
    int	  nbytes = GET_TUPLE_SLOT_AS_INT( arg, 3 );	// Number of bytes to read.


    int		flag = 0;
    int		n;

    if (GET_TUPLE_SLOT_AS_VAL(arg, 4) == HEAP_TRUE) flag |= MSG_OOB;
    if (GET_TUPLE_SLOT_AS_VAL(arg, 5) == HEAP_TRUE) flag |= MSG_PEEK;

    // We cannot reference anything on the Mythryl
    // heap after we do RELEASE_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_foo into C storage: 
    //
    Mythryl_Heap_Value_Buffer  readbuf_buf;
    //
    {   char* c_readbuf =  buffer_mythryl_heap_nonvalue( &readbuf_buf, nbytes );

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_recvbuf", arg );
	    //
	/*  do { */						// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c
		//
		n = recv (socket, c_readbuf, nbytes, flag);
		//
	/*  } while (n < 0 && errno == EINTR);	*/		// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_recvbuf" );

	// Copy bytes read from readbuf_buf
	// onto the given Mythryl-heap buffer:
	//
        Val   buf      = GET_TUPLE_SLOT_AS_VAL( arg, 1 );	// We fetch this after the call, since the heapcleaner may move it during the call.
        char* bufstart = HEAP_STRING_AS_C_STRING(buf) + offset;
	memcpy( bufstart, c_readbuf, n );

	unbuffer_mythryl_heap_value( &readbuf_buf );
    }

    CHECK_RETURN (task, n)
}



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

