// accept.c

#include "../../config.h"

#include <errno.h>

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



Val   _lib7_Sock_accept   (Task* task,  Val arg)   {
    //=================
    //
    // Mythryl type:   Socket -> (Socket, Address)
    //
    // This fn gets bound as   accept'   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

    int		socket = TAGGED_INT_TO_C_INT(arg);
    char	address_buf[  MAX_SOCK_ADDR_BYTESIZE ];
    socklen_t	address_len = MAX_SOCK_ADDR_BYTESIZE;
    int		new_socket;

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c	*/

        new_socket = accept (socket, (struct sockaddr*) address_buf, &address_len);

/*  } while (new_socket < 0 && errno == EINTR);	*/		/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

    if (new_socket == -1) {
        //
	return  RAISE_SYSERR( task, new_socket );
        //
    } else {
        //
	Val data =  make_int64_vector_sized_in_bytes( task, address_buf, address_len );

	Val                address;
	SEQHDR_ALLOC(task, address, UNT8_RO_VECTOR_TAGWORD, data, address_len);

	Val              result;
	REC_ALLOC2(task, result, TAGGED_INT_FROM_C_INT( new_socket ), address);
	return           result;
    }
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

