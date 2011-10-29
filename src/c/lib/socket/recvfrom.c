// recvfrom.c


#include "../../mythryl-config.h"

#include <errno.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/*
###           "Heavier-than-air flying machines are impossible."
###                              -- Lord Kelvin, 1895
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_recvfrom   (Task* task,  Val arg)   {
    //===================
    //
    // Mythryl type is:   (Socket, Int, Bool, Bool) -> (vector_of_one_byte_unts::Vector, Internet_Address)
    //
    // The arguments are:
    //      socket,
    //      number of bytes,
    //      OOB flag
    //      peek flag
    //
    //  The result is the vector of bytes read,
    //  and the source address.
    //
    // This fn gets bound as   recv_from_v'   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg


    char addrBuf[           MAX_SOCK_ADDR_BYTESIZE ];
    socklen_t address_len = MAX_SOCK_ADDR_BYTESIZE;

    int                                                          flag  = 0;
    int  socket =  GET_TUPLE_SLOT_AS_INT( arg, 0 );
    int  nbytes =  GET_TUPLE_SLOT_AS_INT( arg, 1 );
    if (           GET_TUPLE_SLOT_AS_VAL( arg, 2 ) == HEAP_TRUE) flag |= MSG_OOB;
    if (           GET_TUPLE_SLOT_AS_VAL( arg, 3 ) == HEAP_TRUE) flag |= MSG_PEEK;

    // Allocate the vector.
    // Note that this might cause a clean, moving things around:
    //
    Val vec = allocate_nonempty_int1_vector( task, BYTES_TO_WORDS(nbytes) );

    int n;

/*  do { */							// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

	n = recvfrom (
		socket,
		PTR_CAST (char*, vec),
		nbytes,
		flag,
		(struct sockaddr *)addrBuf,
		&address_len
	    );

/*  } while (n < 0 && errno == EINTR);	*/			// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    if (n < 0)     return RAISE_SYSERR(task, status);


    Val	data =  make_int2_vector_sized_in_bytes( task, addrBuf, address_len );
    Val	result;

    if (n == 0) {
	//
	result = ZERO_LENGTH_STRING_GLOBAL;

    } else {

	if (n < nbytes) {
	    shrink_fresh_int1_vector( task, vec, BYTES_TO_WORDS(n) );		// Shrink the vector.
	}
	SEQHDR_ALLOC (task, result, STRING_TAGWORD, vec, n);
    }

    Val	                addr;
    SEQHDR_ALLOC( task, addr, UNT8_RO_VECTOR_TAGWORD, data, address_len);
    REC_ALLOC2(   task, result, result, addr);

    return result;
}



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

