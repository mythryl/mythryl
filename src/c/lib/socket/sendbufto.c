// sendbufto.c


#include "../../mythryl-config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



/*
###                         "The Americans are good about making fancy cars and refrigerators,
###                          but that doesn't mean they are any good at making aircraft.
###                          They are bluffing. They are excellent at bluffing."
###
###                                   -- Hermann Goering, Commander-in-Chief of the Luftwaffe, 1942
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_sendbufto   (Task* task,  Val arg)   {
    //====================
    //
    // Mythryl type:   (Socket, Bytes, Int, Int, Bool, Bool, Addr) -> Int
    //
    // Send data from the buffer; bytes is either
    // a rw_vector_of_one_byte_unts::Rw_Vector,
    // or a vector_of_one_byte_unts::Vector.
    //
    // The arguments are:
    //     socket
    //     data buffer
    //     start position
    //     number of bytes
    //     OOB flag
    //     don't_route flag
    //     destination address
    //
    // This fn gets bound as   send_to_v, send_to_a   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

    int	  socket =                                 GET_TUPLE_SLOT_AS_INT( arg, 0 );
    Val	  buf    =                                 GET_TUPLE_SLOT_AS_VAL( arg, 1 );
    char* data   =  HEAP_STRING_AS_C_STRING(buf) + GET_TUPLE_SLOT_AS_INT( arg, 2 );
    int	  nbytes =                                 GET_TUPLE_SLOT_AS_INT( arg, 3 );           int flgs  = 0;
    if                                            (GET_TUPLE_SLOT_AS_VAL( arg, 4 ) == HEAP_TRUE)  flgs |= MSG_OOB;
    if                                            (GET_TUPLE_SLOT_AS_VAL( arg, 5 ) == HEAP_TRUE)  flgs |= MSG_DONTROUTE;
    Val	  addr   =                                 GET_TUPLE_SLOT_AS_VAL( arg, 6 );

    int n;

/*  do { */						// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

	n = sendto (
		socket,
		data,
		nbytes,
		flgs,
		GET_VECTOR_DATACHUNK_AS (struct sockaddr*, addr ),
		GET_VECTOR_LENGTH( addr )
	    );

/*  } while (n < 0 && errno == EINTR);	*/	// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    CHECK_RETURN (task, n);

}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

