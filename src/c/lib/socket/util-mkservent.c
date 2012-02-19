// util-mkservent.c


#include "../../mythryl-config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "socket-util.h"
#include "heap.h"



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

    // If our agegroup0 buffer is more than half full,
    // empty it by doing a heapcleaning.  This is very
    // conservative -- which is the way I like it. :-)
    //
    if (agegroup0_freespace_in_bytes( task )
      < agegroup0_usedspace_in_bytes( task )
    ){
	call_heapcleaner( task,  0 );
    }

    // Build the return result:

    Val name    =  make_ascii_string_from_c_string__may_heapclean(		task, sentry->s_name,     NULL	);		Roots roots1 = { &name,	    NULL };
    Val aliases =  make_ascii_strings_from_vector_of_c_strings__may_heapclean(	task, sentry->s_aliases, &roots1);		Roots roots2 = { &aliases,  &roots1 };
    Val proto   =  make_ascii_string_from_c_string__may_heapclean(		task, sentry->s_proto,   &roots2);		Roots roots3 = { &proto,    &roots2 };
    Val port    =  TAGGED_INT_FROM_C_INT(					ntohs(sentry->s_port)		);

    Val result  =  make_four_slot_record(task,  name, aliases, port, proto);

    return OPTION_THE( task, result );
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

