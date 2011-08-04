// util-mkservent.c


#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "socket-util.h"



/*
###             "Rail travel at high speeds is not possible because
###              passengers, unable to breathe, would die of asphyxia."
###
###                           -- Dionysius Lardner, 1830,
###                              Professor of Natural Philosophy and Astronomy at University College, London,
###                              author of The Steam Engine Explained and Illustrated.
###
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _util_NetDB_mkservent   (Task* task,  struct servent* sentry)   {
    //=====================
    //
    // Mythryl type:
    //
    // Allocate an Lib7 value of type:
    //    Null_Or(   (String, List(String), Int, String)   )
    // to represent a struct servent value.  Note that the port number is returned
    // in network byteorder, so we need to map it to host order.


    if (sentry == NULL)   return OPTION_NULL;

    // Build the return result:

    Val name    = make_ascii_string_from_c_string(     task, sentry->s_name	);
    Val aliases = make_ascii_strings_from_vector_of_c_strings( task, sentry->s_aliases	);
    Val proto   = make_ascii_string_from_c_string(     task, sentry->s_proto	);
    Val port    = INT31_FROM_C_INT(      ntohs(sentry->s_port)	);

    Val	               result;
    REC_ALLOC4(  task, result, name, aliases, port, proto);
    OPTION_THE( task, result, result);
    return             result;
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

