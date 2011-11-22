// bind.c


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



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_bind   (Task* task,  Val arg)   {
    //===============
    //
    // Mythryl type: (Socket, Addr) -> Void
    //
    // This function gets bound as   bind'   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

    int		socket = GET_TUPLE_SLOT_AS_INT(arg, 0);
    Val	addr = GET_TUPLE_SLOT_AS_VAL(arg, 1);

    struct sockaddr* heap_sockaddr = GET_VECTOR_DATACHUNK_AS( struct sockaddr*, addr );
    int              addr_len      = GET_VECTOR_LENGTH( addr );

    struct sockaddr c_sockaddr = *heap_sockaddr;			// May not reference Mythryl heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP, so make copy on C stack.

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_bind", arg );
	//
	int status = bind (socket, &c_sockaddr, addr_len);
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_bind" );

    CHECK_RETURN_UNIT(task, status);
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

