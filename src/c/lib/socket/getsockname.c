// getsockname.c

#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



/*
###              "The radio craze will die out in time."
###                               -- Thomas Edison, 1922
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_getsockname   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type:   Socket -> Addr
    //
    // This fn gets bound as   get_sock_name'   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Sock_getsockname");

    int		socket = TAGGED_INT_TO_C_INT(arg);

    char	address_buf[  MAX_SOCK_ADDR_BYTESIZE ];
    socklen_t	address_len = MAX_SOCK_ADDR_BYTESIZE;

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_getsockname", arg );
	//
	int status = getsockname (socket, (struct sockaddr*) address_buf, &address_len);
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_getsockname" );

    if (status == -1)   return  RAISE_SYSERR( task, status );


    Val	data
	=
	make_int2_vector_sized_in_bytes(				// make_int2_vector_sized_in_bytes	def in    src/c/heapcleaner/make-strings-and-vectors-etc.c
	    //
	    task,
	    address_buf,
	    address_len
	);

    Val	                address;
    SEQHDR_ALLOC( task, address, UNT8_RO_VECTOR_TAGWORD, data, address_len );
    return              address;
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

