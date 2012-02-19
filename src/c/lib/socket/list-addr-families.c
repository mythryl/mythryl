// list-addr-families.c


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "make-strings-and-vectors-etc.h"
#include "socket-util.h"
#include "cfun-proto-list.h"
#include "raise-error.h"



/*
###             "Atomic energy might be as good as our present-day explosives,
###              but it is unlikely to produce anything very much more dangerous."
###
###                                -- Winston Churchill, 1939
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_listaddrfamilies   (Task* task,  Val arg)   {
    //===========================
    //
    // Mythryl type:
    //
    // Return a list of the known address families.
    // (This may contain unsupported families.)
    //
    // This function gets imported into the Mythryl world via:
    //     src/lib/std/src/socket/socket-guts.pkg
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Sock_listaddrfamilies");

    return dump_table_as_system_constants_list__may_heapclean (task, &_Sock_AddrFamily, NULL);
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

