// get-host-by-name.c


#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "socket-util.h"

/*
###             "[It would not be long] ere the whole surface
###              of this country would be channelled for those
###              nerves which are to diffuse, with the speed
###              of thought, a knowledge of all that is occurring
###              throughout the land, making, in fact,
###              one neighborhood of the whole country."
###
###                                  -- Samuel Morse, 1838
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_netdb_get_host_by_name   (Task* task,  Val arg)   {
    //============================
    //
    // Mythryl type:   String -> Null_Or(   (String, List(String), Raw_Address_Family, List( Internet_Address ))   )
    //
    // This fn gets bound as   get_host_by_name'   in:
    //
    //     src/lib/std/src/socket/dns-host-lookup.pkg

    return  _util_NetDB_mkhostent (									// _util_NetDB_mkhostent	def in    src/c/lib/socket/util-mkhostent.c
		//
		task,
		gethostbyname( HEAP_STRING_AS_C_STRING( arg ) )
	    );
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

