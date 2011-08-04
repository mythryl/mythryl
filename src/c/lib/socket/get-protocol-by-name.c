// get-protocol-by-name.c


#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "socket-util.h"

/*
###                     "It is now possible for a business man to
###                      talk with his office from a moving vehicle.
###                      The apparatus necessary to do this marvellous
###                      thing can be carried in a small dress suit case."
###
###                                          -- John Brady, 1920
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_netdb_get_protocol_by_name   (Task* task,  Val arg)   {
    //=========================
    //
    // Mythryl type:   String -> Null_Or(   (String, List(String), Int)   )
    //
    // This fn gets bound as   get_prot_by_name'   in:
    // 
    //     src/lib/std/src/socket/net-protocol-db.pkg

    struct protoent*  pentry
	=
	getprotobyname (HEAP_STRING_AS_C_STRING(arg));

    if (pentry == NULL)   return OPTION_NULL;


    Val name    =  make_ascii_string_from_c_string (task, pentry->p_name);
    Val aliases =  make_ascii_strings_from_vector_of_c_strings (task, pentry->p_aliases);

    Val                result;
    REC_ALLOC3(  task, result, name, aliases, INT31_FROM_C_INT(pentry->p_proto));
    OPTION_THE( task, result, result);
    return             result;
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

