// get-service-by-name.c


#include "../../mythryl-config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "socket-util.h"

/*
###           "I see little commercial potential for
###            the Internet for at least ten years."
###
###                         -- Bill Gates, 1994
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_netdb_get_service_by_name   (Task* task,  Val arg)   {
    //===============================
    //
    // Mythryl type:   (String, Null_Or(String)) ->   Null_Or(   (String, List(String), Int, String)   )
    //
    // This fn gets bound as   get_service_by_name'   in:
    //
    //     src/lib/std/src/socket/net-service-db.pkg

    Val	mlServ  =  GET_TUPLE_SLOT_AS_VAL( arg, 0 );
    Val	mlProto =  GET_TUPLE_SLOT_AS_VAL( arg, 1 );

    char* proto;

    if (mlProto == OPTION_NULL)   proto = NULL;
    else			  proto = HEAP_STRING_AS_C_STRING(OPTION_GET(mlProto));

    return _util_NetDB_mkservent (						// _util_NetDB_mkservent	def in   src/c/lib/socket/util-mkservent.c
	       task,
	       getservbyname( HEAP_STRING_AS_C_STRING( mlServ ), proto)
           );
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

