// getrpcbynum.c


#include "../../config.h"

#include "system-dependent-unix-stuff.h"
#include "sockets-osdep.h"
#include <netdb.h>
#ifdef INCLUDE_RPCENT_H
#  include INCLUDE_RPCENT_H
#  ifdef Bool		// NetBSD hack
#    undef Bool
#  endif
#endif
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "socket-util.h"



/*
###               "While theoretically television may be feasible,
###                commercially and financially I consider it an
###                impossibility, a development of which we need
###                waste little time dreaming."
###
###                                 -- Lee de Forest, 1926
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_NetDB_getrpcbynum   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:  Int ->   Null_Or(   (String, List(String), Int)   )
    //
    // This fn is NOWHERE INVOKED.  Nor listed in   src/c/lib/socket/cfun-list.h   Presumably should be either called or deleted:  XXX BUGGO FIXME.

    struct rpcent*  rentry
	=
        getrpcbynumber( INT31_TO_C_INT( arg ));

    if (rentry == NULL)   return OPTION_NULL;

    Val name    =  make_ascii_string_from_c_string(     task, rentry->r_name   );
    Val aliases =  make_ascii_strings_from_vector_of_c_strings( task, rentry->r_aliases);

    Val                result;
    REC_ALLOC3(  task, result, name, aliases, INT31_FROM_C_INT(rentry->r_number));
    OPTION_THE( task, result, result);
    return             result;
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

