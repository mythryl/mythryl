// util-mkhostent.c


#include "../../mythryl-config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "socket-util.h"
#include "heap.h"



/*
###                        "Louis Pasteur's theory of germs is ridiculous fiction."
###                                 -- Pierre Pachet, 1872,
###                                    professor of physiology at Toulouse
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _util_NetDB_mkhostent   (Task* task,  struct hostent* hentry)   {
    //=====================
    //
    // Mythryl type:
    //
    // Allocate a Mythryl value of type
    //    Null_Or(   (String, List(String), Addr_Family, List(Addr))   )
    // to represent a struct hostent value.
    //
    // NOTE: we should probably be passing back the value of h_errno, but this
    // will require an API change at the Lib7 level.  XXX BUGGO FIXME

    if (hentry == NULL)   return OPTION_NULL;


    // Build the return result:

    Val	addr;

    int	nAddresses;
 
    Val	addresses = LIST_NIL;																Roots roots0 = { &addresses, NULL };

    Val name    =  make_ascii_string_from_c_string__may_heapclean(		task,                    hentry->h_name,	 NULL	);		Roots roots1 = { &name,    &roots0 };
    Val aliases =  make_ascii_strings_from_vector_of_c_strings__may_heapclean(	task,                    hentry->h_aliases,	&roots1	);		Roots roots2 = { &aliases, &roots1 };
    Val af      =  make_system_constant__may_heapclean(				task, &_Sock_AddrFamily, hentry->h_addrtype );				Roots roots3 = { &af,      &roots2 };

    for (nAddresses = 0;  hentry->h_addr_list[nAddresses] != NULL;  nAddresses++);

    for (int i = nAddresses;  --i >= 0;  ) {
        //
	addr = allocate_nonempty_ascii_string__may_heapclean (task, hentry->h_length);

	memcpy (GET_VECTOR_DATACHUNK_AS(void*, addr), hentry->h_addr_list[i], hentry->h_length);

	addresses = LIST_CONS( task, addr, addresses );

	// If our agegroup0 buffer is more than half full,
	// empty it by doing a heapcleaning.  This is very
	// conservative -- which is the way I like it. :-)
	//
	if (agegroup0_freespace_in_bytes( task )
	  < agegroup0_usedspace_in_bytes( task )
	){
	    call_heapcleaner_with_extra_roots( task,  0, &roots3 );
	}
    }

    Val result =  make_four_slot_record(task,  name, aliases, af, addresses);

    return   OPTION_THE( task, result );
}



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

