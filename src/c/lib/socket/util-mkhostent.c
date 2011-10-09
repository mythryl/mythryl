// util-mkhostent.c


#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "socket-util.h"



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
    Val	result;

    int	nAddresses;
 
    Val	addresses = LIST_NIL;

    Val name    =  make_ascii_string_from_c_string(                     task,                    hentry->h_name		);
    Val aliases =  make_ascii_strings_from_vector_of_c_strings( task,                    hentry->h_aliases	);
    Val af      =  make_system_constant(                                task, &_Sock_AddrFamily, hentry->h_addrtype	);

    for (nAddresses = 0;  hentry->h_addr_list[nAddresses] != NULL;  nAddresses++);

    for (int i = nAddresses;  --i >= 0;  ) {
        //
	addr = allocate_nonempty_ascii_string (task, hentry->h_length);

	memcpy (GET_VECTOR_DATACHUNK_AS(void*, addr), hentry->h_addr_list[i], hentry->h_length);

	LIST_CONS( task, addresses, addr, addresses );
    }

    REC_ALLOC4 (task, result, name, aliases, af, addresses);

    OPTION_THE (task, result, result);

    return result;
}



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

