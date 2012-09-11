// get-host-by-address.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <netdb.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"
#include "socket-util.h"

/*
###         "So many centuries after the Creation,
###          it is unlikely that anyone could find
###          hitherto unknown lands of any value."
###             -- report to King Ferdinand and
###                Queen Isabella of Spain, 1486
*/ 



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_netdb_get_host_by_address   (Task* task,  Val arg)   {
    //===============================
    //
    // Mythryl type:   Internet_Address -> Null_Or(  (String, List(String), Raw_Address_Family, List(Internet_Address))  )
    //
    // This fn gets bound as   get_host_by_addr'   in:
    //
    //     src/lib/std/src/socket/dns-host-lookup.pkg

															ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_netdb_get_host_by_address");

    ASSERT (sizeof(struct in_addr) == GET_VECTOR_LENGTH( arg ));

    struct in_addr*  heap_arg =  (struct in_addr*) HEAP_STRING_AS_C_STRING( arg );					// Last use of 'arg'.
    struct in_addr      c_arg = *heap_arg;

    RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_netdb_get_host_by_address", NULL );
	//
	struct hostent* result = gethostbyaddr (&c_arg, sizeof(struct in_addr), AF_INET);
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_netdb_get_host_by_address" );

    return  _util_NetDB_mkhostent ( task, result );									// _util_NetDB_mkhostent	def in    src/c/lib/socket/util-mkhostent.c
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

