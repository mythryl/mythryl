// getpeername.c

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
#include "socket-util.h"



/*
###                 "Stocks have reached what looks like a a permanently high plateau."
###
###                      -- Irving Fisher, Professor of Economics, Yale University, 1929
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_getpeername   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type:   Socket -> (Address_Family, Address)
    //
    // This function gets bound as   get_peer_name'   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Sock_getpeername");

    char addr[ MAX_SOCK_ADDR_BYTESIZE ];

    socklen_t  address_len =  MAX_SOCK_ADDR_BYTESIZE;

    int sockfd = TAGGED_INT_TO_C_INT( arg );

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_getpeername", arg );
	//
	int status = getpeername (sockfd, (struct sockaddr *)addr, &address_len);
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_getpeername" );

    if (status < 0)   return RAISE_SYSERR(task, status);

    Val cdata =  make_int2_vector_sized_in_bytes( task, addr, address_len );

    return  make_vector_header(task,  UNT8_RO_VECTOR_TAGWORD, cdata, address_len);
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

