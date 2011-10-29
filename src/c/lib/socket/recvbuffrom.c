// recvbuffrom.c

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

    char       address_buf[  MAX_SOCK_ADDR_BYTESIZE ];
    socklen_t  address_len = MAX_SOCK_ADDR_BYTESIZE;

    int	  socket  = GET_TUPLE_SLOT_AS_INT(                   arg, 0);
    Val   buf     = GET_TUPLE_SLOT_AS_VAL(                      arg, 1);
    char* start   = HEAP_STRING_AS_C_STRING(buf) + GET_TUPLE_SLOT_AS_INT(arg, 2);
    int	  nbytes  = GET_TUPLE_SLOT_AS_INT(                   arg, 3);

    int	  flag = 0;

    int	   n;

    if (GET_TUPLE_SLOT_AS_VAL(arg, 4) == HEAP_TRUE) flag |= MSG_OOB;
    if (GET_TUPLE_SLOT_AS_VAL(arg, 5) == HEAP_TRUE) flag |= MSG_PEEK;

/*  do { */					// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

        n = recvfrom( socket,
                      start,
                      nbytes,
                      flag,
                      (struct sockaddr *)address_buf,
                      &address_len
                    );

/*  } while (n < 0 && errno == EINTR);	*/	// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    if (n < 0)   return RAISE_SYSERR(task, status);

    Val	data =  make_int2_vector_sized_in_bytes( task, address_buf, address_len );

    Val	                address;
    SEQHDR_ALLOC (task, address, UNT8_RO_VECTOR_TAGWORD, data, address_len);

    Val	             result;
    REC_ALLOC2(task, result, TAGGED_INT_FROM_C_INT(n), address);
    return           result;
}



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

