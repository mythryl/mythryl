// get-host-by-name.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <netdb.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "raise-error.h"
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



//       struct hostent *gethostbyname(const char *name);



Val   _lib7_netdb_get_host_by_name   (Task* task,  Val arg)   {
    //============================
    //
    // Mythryl type:   String -> Null_Or(   (String, List(String), Raw_Address_Family, List( Internet_Address ))   )
    //
    // This fn gets bound as   get_host_by_name'   in:
    //
    //     src/lib/std/src/socket/dns-host-lookup.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_netdb_get_host_by_name");

    char* heap_name = HEAP_STRING_AS_C_STRING( arg );

    struct hostent* result;

    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_path into C storage: 
    //
    Mythryl_Heap_Value_Buffer  name_buf;
    //
    {	char* c_name
	    = 
	    buffer_mythryl_heap_value( &name_buf, (void*) heap_name, strlen( heap_name ) +1 );		// '+1' for terminal NUL on string.


	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_netdb_get_host_by_name", arg );
	    //
	    result = gethostbyname( c_name );
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_netdb_get_host_by_name" );

	unbuffer_mythryl_heap_value( &name_buf );
    }

    return  _util_NetDB_mkhostent (task, result);								// _util_NetDB_mkhostent	def in    src/c/lib/socket/util-mkhostent.c
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

