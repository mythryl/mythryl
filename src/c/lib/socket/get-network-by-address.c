// get-network-by-address.c


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

#if defined(__CYGWIN32__)
#undef getnetbyaddr
#define getnetbyaddr(x,y) NULL
#endif


/*
###                  "This 'telephone' has too many shortcomings
###                   to be seriously considered as a means of
###                   communication. The device is inherently
###                   of no value to us."
###
###                             -- Western Union memo, 1865
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



//     struct netent *getnetbyaddr(uint32_t net, int type);


Val   _lib7_netdb_get_network_by_address   (Task* task,  Val arg)   {
    //==================================
    //
    // Mythryl type:   (Sysword, Address_Family) -> Null_Or(  (String, List(String), Address_Family, Sysword)  )
    //
    // This fn gets bound as   get_network_by_address'   in:
    //
    //     src/lib/std/src/socket/net-db.pkg

												ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Val result;

    #if defined(OPSYS_WIN32)
        // XXX BUGGO FIXME:
	//     getnetbyaddr() does not seem to exist under Windows.
	// What is the equivalent?
	result = RAISE_ERROR__MAY_HEAPCLEAN(task, "<getnetbyaddr not implemented>", NULL);
    #else
	unsigned long   net  =  TUPLE_GETWORD(         arg, 0 );
	int		type =  GET_TUPLE_SLOT_AS_INT( arg, 1 );				// Last use of 'arg'.

	RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	    //
	    struct netent* resultt =  getnetbyaddr(net, type);
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

	result = _util_NetDB_mknetent (task, resultt);
    #endif

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

