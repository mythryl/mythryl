// recvbuffrom.c

#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/*
###           "Never underestimate the bandwidth
###            of a station wagon full of tapes
###            hurtling down the highway."
###
###                        -- Andrew Tannenbaum
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_recvbuffrom   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type:   (Socket, rw_vector_of_one_byte_unts::Rw_Vector, Int, Int, Bool, Bool) -> (Int, Addr)
    //
    // The arguments are:
    //     socket,
    //     data buffer,
    //     start position,
    //     number of bytes,
    //     OOB flag
    //     peek flag.
    //
    // The result is:
    //     number of bytes read
    //     source address.
    //
    // This fn gets bound as   recv_from_a   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Sock_recvbuffrom");

    char       address_buf[  MAX_SOCK_ADDR_BYTESIZE ];
    socklen_t  address_len = MAX_SOCK_ADDR_BYTESIZE;

    int	  socket  = GET_TUPLE_SLOT_AS_INT( arg, 0);
//  Val   buf     = GET_TUPLE_SLOT_AS_VAL( arg, 1);		// Mythryl buffer to read bytes into.	// We'll fetch this after the call, since it may move around during the call.
    int   offset  = GET_TUPLE_SLOT_AS_INT( arg, 2);		// Offset within buf to read bytes into.
    int	  nbytes  = GET_TUPLE_SLOT_AS_INT( arg, 3);		// Number of bytes to read.

    int	  flag = 0;

    int	   n;

    if (GET_TUPLE_SLOT_AS_VAL(arg, 4) == HEAP_TRUE) flag |= MSG_OOB;
    if (GET_TUPLE_SLOT_AS_VAL(arg, 5) == HEAP_TRUE) flag |= MSG_PEEK;

    // We cannot reference anything on the Mythryl heap
    // between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so allocate a C-side read buffer:
    //
    Mythryl_Heap_Value_Buffer  readbuf_buf;
    //
    {   char* c_readbuf =  buffer_mythryl_heap_nonvalue( &readbuf_buf, nbytes );

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_recvbuffrom", arg );
	    //
	    /*  do { */					// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

		    n = recvfrom( socket,
				  c_readbuf,
				  nbytes,
				  flag,
				  (struct sockaddr *)address_buf,
				  &address_len
				);

	    /*  } while (n < 0 && errno == EINTR);	*/	// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_recvbuffrom" );

	if (n < 0) {
	    unbuffer_mythryl_heap_value( &readbuf_buf );
	    return RAISE_SYSERR(task, status);
	}

	Val   buf      =  GET_TUPLE_SLOT_AS_VAL( arg, 1);		// Mythryl buffer to read bytes into.	// We'll fetch this after the call, since it may move around during the call.
	char* bufstart =  HEAP_STRING_AS_C_STRING(buf) + offset;

	memcpy( bufstart, c_readbuf, n);

	unbuffer_mythryl_heap_value( &readbuf_buf );
    }


    Val	data    =  make_biwordslots_vector_sized_in_bytes__may_heapclean( task, address_buf,                   address_len );
    Val address =  make_vector_header(              task,  UNT8_RO_VECTOR_TAGWORD, data, address_len);

    return  make_two_slot_record(task,  TAGGED_INT_FROM_C_INT(n), address);
}



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

