// getsockname.c

#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
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

														ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int		socket = TAGGED_INT_TO_C_INT( arg );								// Last use of 'arg'.

    char	address_buf[  MAX_SOCK_ADDR_BYTESIZE ];
    socklen_t	address_len = MAX_SOCK_ADDR_BYTESIZE;

    RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_Sock_getsockname", NULL );
	//
	int status = getsockname (socket, (struct sockaddr*) address_buf, &address_len);
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_Sock_getsockname" );

    if (status == -1)   return  RAISE_SYSERR__MAY_HEAPCLEAN(task, status, NULL);

														// make_biwordslots_vector_sized_in_bytes__may_heapclean	def in    src/c/heapcleaner/make-strings-and-vectors-etc.c
    Val	data = make_biwordslots_vector_sized_in_bytes__may_heapclean(task, address_buf, address_len, NULL );

    return  make_vector_header( task,  UNT8_RO_VECTOR_TAGWORD, data, address_len );
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

