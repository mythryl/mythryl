// getpeername.c

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

												ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    char addr[ MAX_SOCK_ADDR_BYTESIZE ];

    socklen_t  address_len =  MAX_SOCK_ADDR_BYTESIZE;

    int sockfd = TAGGED_INT_TO_C_INT( arg );							// Last use of 'arg'.

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	int status = getpeername (sockfd, (struct sockaddr *)addr, &address_len);
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    if (status < 0)   return RAISE_SYSERR__MAY_HEAPCLEAN(task, status, NULL);

    Val cdata =  make_biwordslots_vector_sized_in_bytes__may_heapclean( task, addr, address_len, NULL );

    Val result =  make_vector_header(task,  UNT8_RO_VECTOR_TAGWORD, cdata, address_len);

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

