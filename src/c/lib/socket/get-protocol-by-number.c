// get-protocol-by-number.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <netdb.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "socket-util.h"



/*
###               "There is not the slightest indication that nuclear energy
###                will ever be obtainable. It would mean the atom would have
###                to be shattered at will."
###
###                                    -- Albert Einstein, 1932
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_netdb_get_protocol_by_number   (Task* task,  Val arg)   {
    //==================================
    //
    // Mythryl type:  Int -> Null_Or(  (String, List(String), Int)   )
    //
    // This fn gets bound as   get_prot_by_number'   in:
    //
    //     src/lib/std/src/socket/net-protocol-db.pkg

    int number = TAGGED_INT_TO_C_INT( arg );

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_netdb_get_protocol_by_number", arg );
	//
	struct protoent*  pentry =   getprotobynumber( number );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_netdb_get_protocol_by_number" );


    if (pentry == NULL)   return OPTION_NULL;


    Val name    = make_ascii_string_from_c_string (task, pentry->p_name);

    Val aliases = make_ascii_strings_from_vector_of_c_strings (task, pentry->p_aliases);

    Val                result;
    REC_ALLOC3(  task, result, name, aliases, TAGGED_INT_FROM_C_INT(pentry->p_proto));
    OPTION_THE( task, result, result);
    return             result;
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

