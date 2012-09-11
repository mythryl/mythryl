// get-network-by-name.c


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

#if defined(__CYGWIN32__)
    #undef getnetbyname
    #define getnetbyname(x) NULL
#endif

/*
###             "Any two friends living within the radius of sensibility
###              of their [electrical ray] receiving instruments, having
###              first decided on their special wave length and attuned
###              their respective instruments to mutual receptivity,
###              could thus communicate as long and as often as they
###              pleased by timing the impulses to produce long and short
###              intervals on the ordinary Morse code."
###
###                                    -- William Crookes, 1892
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



//       struct netent *getnetbyname(const char *name);


Val   _lib7_netdb_get_network_by_name   (Task* task,  Val arg)   {
    //===============================
    //
    // Mythryl type: String -> Null_Or(   (String, List(String), Addr_Family, Sysword)   )
    //
    // This fn gets bound as   get_network_by_name'   in:
    //
    //     src/lib/std/src/socket/net-db.pkg

															ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_netdb_get_network_by_name");

    #if defined(OPSYS_WIN32)
        // XXX BUGGO FIXME:  getnetbyname() does not seem to exist under Windows.  What is the equivalent?
        return RAISE_ERROR__MAY_HEAPCLEAN(task, "<getnetbyname not implemented>", NULL);
    #else
	struct netent* result;

	char* heap_name = HEAP_STRING_AS_C_STRING( arg );								// Last use of 'arg'.

	// We cannot reference anything on the Mythryl
	// heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
	// because garbage collection might be moving
	// it around, so copy heap_path into C storage: 
	//
	Mythryl_Heap_Value_Buffer  name_buf;
	//
	{   char* c_name =  buffer_mythryl_heap_value( &name_buf, (void*) heap_name, strlen( heap_name ) +1 );		// '+1' for terminal NUL on string.

	    RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_netdb_get_network_by_name", NULL );
		//
		result = getnetbyname( c_name );
		//
	    RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_netdb_get_network_by_name" );

	    unbuffer_mythryl_heap_value( &name_buf );
	}

	return _util_NetDB_mknetent (task, result);
    #endif
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

