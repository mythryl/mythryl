// getaddrfamily.c


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



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_getaddrfamily   (Task* task,  Val arg)   {
    //========================
    //
    // Mythryl type:   Internet_Address -> Raw_Address_Family
    //
    // Extract the family field, convert to host byteorder, and return it.
    //
    // This fn is bound as   get_address_family   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    struct sockaddr*  addr =  GET_VECTOR_DATACHUNK_AS( struct sockaddr*, arg );

    Val result =   make_system_constant__may_heapclean( task, &_Sock_AddrFamily, ntohs(addr->sa_family), NULL );

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

