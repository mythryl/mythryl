// getrpcbynum.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <netdb.h>

#include "system-dependent-unix-stuff.h"
#include "sockets-osdep.h"
#ifdef INCLUDE_RPCENT_H
#  include INCLUDE_RPCENT_H
#  ifdef Bool		// NetBSD hack
#    undef Bool
#  endif
#endif
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
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
    // This fn is NOWHERE INVOKED.  Nor listed in   src/c/lib/socket/cfun-list.h   Presumably should be either called or deleted:  XXX SUCKO FIXME.

												ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    struct rpcent*  rentry;

    int number = TAGGED_INT_TO_C_INT( arg );							// Last use of 'arg'.

    RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_NetDB_getrpcbynum", NULL );
	//
	rentry = getrpcbynumber( number );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_NetDB_getrpcbynum" );

    if (rentry == NULL)   return OPTION_NULL;

    Val name    =  make_ascii_string_from_c_string__may_heapclean(		task, rentry->r_name,	  NULL	    );					Roots roots1 = { &name, NULL };
    Val aliases =  make_ascii_strings_from_vector_of_c_strings__may_heapclean(	task, rentry->r_aliases, &roots1    );

    Val result  =  make_three_slot_record(					task,  name,  aliases,  TAGGED_INT_FROM_C_INT(rentry->r_number)  );

    return   OPTION_THE( task, result );
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

