// util-mknetent.c


#include "../../mythryl-config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "socket-util.h"



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


    Val name    =  make_ascii_string_from_c_string(             task,                    nentry->n_name		);
    Val aliases =  make_ascii_strings_from_vector_of_c_strings( task,                    nentry->n_aliases	);
    Val af      =  make_system_constant(                        task, &_Sock_AddrFamily, nentry->n_addrtype	);

    Val net     =  make_one_word_unt(                           task,  (Val_Sized_Unt) (nentry->n_net)  );

    Val	result  =  make_four_slot_record(task,  name, aliases, af, net  );

    return   OPTION_THE( task, result );
}



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

