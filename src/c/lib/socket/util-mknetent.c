// util-mknetent.c


#include "../../mythryl-config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "socket-util.h"
#include "heap.h"



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _util_NetDB_mknetent   (Task *task, struct netent* nentry)   {
    //====================
    //
    // Allocate a Mythryl value of type
    //    Null_Or(   (String, List(String), Addr_Family, Sysword)   )
    // to represent a struct netent value.

    if (nentry == NULL)   return OPTION_NULL;

    // Build the return result:

    // If our agegroup0 buffer is more than half full,
    // empty it by doing a heapcleaning.  This is very
    // conservative -- which is the way I like it. :-)
    //
    if (agegroup0_freespace_in_bytes( task )
      < agegroup0_usedspace_in_bytes( task )
    ){
	call_heapcleaner( task, 0 );
    }

    Val name    =  make_ascii_string_from_c_string__may_heapclean(		task,                    nentry->n_name,     NULL	);		Roots roots1 = { &name,    NULL	    };
    Val aliases =  make_ascii_strings_from_vector_of_c_strings__may_heapclean(	task,                    nentry->n_aliases,  &roots1	);		Roots roots2 = { &aliases, &roots1  };
    Val af      =  make_system_constant__may_heapclean(				task, &_Sock_AddrFamily, nentry->n_addrtype, &roots2	);	//	Roots roots3 = { &af,      &roots2  };
    Val net     =  make_one_word_unt(						task,  (Vunt) (nentry->n_net)			);	//	Roots roots4 = { &net,	   &roots3  };

    Val	result  =  make_four_slot_record(					task,  name, aliases, af, net  );

    return   OPTION_THE( task, result );
}



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

