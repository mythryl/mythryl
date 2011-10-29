// recvbuf.c


#include "../../mythryl-config.h"

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

    int	  socket = GET_TUPLE_SLOT_AS_INT(                                arg, 0 );
    Val	  buf    = GET_TUPLE_SLOT_AS_VAL(                                arg, 1 );
    char* start  = HEAP_STRING_AS_C_STRING(buf) + GET_TUPLE_SLOT_AS_INT( arg, 2 );
    int	  nbytes = GET_TUPLE_SLOT_AS_INT(                                arg, 3 );

    int		flag = 0;
    int		n;

    if (GET_TUPLE_SLOT_AS_VAL(arg, 4) == HEAP_TRUE) flag |= MSG_OOB;
    if (GET_TUPLE_SLOT_AS_VAL(arg, 5) == HEAP_TRUE) flag |= MSG_PEEK;

    /*  do { */						// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

        n = recv (socket, start, nbytes, flag);

/*  } while (n < 0 && errno == EINTR);	*/		// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.


    CHECK_RETURN (task, n)
}



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

