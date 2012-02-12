// recv.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#include "hexdump-if.h"



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_recv   (Task* task,  Val arg)   {
    //===============
    //
    // Mythryl type: (Socket, Int, Bool, Bool) -> vector_of_one_byte_unts::Vector
    //
    // The arguments are: socket, number of bytes, OOB flag and peek flag.
    // The result is the vector of bytes received.
    //
    // This fn gets bound as   recv_v'   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Sock_recv");

    Val vec;
    int n;

    int	socket = GET_TUPLE_SLOT_AS_INT( arg, 0 );
    int	nbytes = GET_TUPLE_SLOT_AS_INT( arg, 1 );
    Val	oob    = GET_TUPLE_SLOT_AS_VAL( arg, 2 );
    Val	peek   = GET_TUPLE_SLOT_AS_VAL( arg, 3 );

    int		           flag  = 0;
    if (oob  == HEAP_TRUE) flag |= MSG_OOB;
    if (peek == HEAP_TRUE) flag |= MSG_PEEK;

    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so allocate a C-world buffer
    // to read the bytes into:
    //
    Mythryl_Heap_Value_Buffer  read_buf;
    //
    {   unsigned char* c_read =  buffer_mythryl_heap_nonvalue( &read_buf, nbytes );
										log_if("recv.c/before: socket d=%d nbytes d=%d oob=%s peek=%s\n",socket,nbytes,(oob == HEAP_TRUE)?"TRUE":"FALSE",(peek == HEAP_TRUE)?"TRUE":"FALSE");
	errno = 0;

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_recv", arg );
	    //
	/*  do { */								// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c
		//
		n = recv (socket, c_read, nbytes, flag);
		//
	/*  } while (n < 0 && errno == EINTR);	*/				// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_recv" );

	if (n <= 0) {
	    unbuffer_mythryl_heap_value( &read_buf );
	    if (n <  0)   return RAISE_SYSERR(task, status);
	    if (n == 0)   return ZERO_LENGTH_STRING__GLOBAL;
	}

	// Allocate result vector to hold the bytes read.
	// NB: This might cause a heapcleaning, moving things around:
	//
	vec = allocate_nonempty_wordslots_vector__may_heapclean( task, BYTES_TO_WORDS(n), NULL );

	// Copy bytes read into result vector:
	//
        memcpy( PTR_CAST(char*, vec), c_read, n);
										log_if(   "recv.c/after: n d=%d errno d=%d (%s)\n", n, errno, errno ? strerror(errno) : "");
										hexdump_if( "recv.c/after: Received data: ", PTR_CAST(unsigned char*, vec), n );
	unbuffer_mythryl_heap_value( &read_buf );
    }

    return make_vector_header( task,  STRING_TAGWORD, vec, n );
}



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

