// get-host-name.c


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

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

/*
###           "Well informed people know it is
###            impossible to transmit the voice
###            over wires and that were it possible
###            to do so, the thing would be of no
###            practical value."
###
###                     -- Boston Post, 1865
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_netdb_get_host_name  (Task* task,  Val arg)   {
    //=========================
    //
    // Mythryl type:   Void -> String
    //
    // This fn gets bound as   get_host_name   in:
    //
    //     src/lib/std/src/socket/dns-host-lookup.pkg

    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_netdb_get_host_name");

    char hostname[ MAXHOSTNAMELEN ];

    RELEASE_MYTHRYL_HEAP( task->pthread, "", arg );
	//
	if (gethostname( hostname, MAXHOSTNAMELEN ) == -1)   return  RAISE_SYSERR( task,  status);
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "" );

    return   make_ascii_string_from_c_string( task, hostname );
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

