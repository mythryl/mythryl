// get-network-by-name.c


#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
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



Val   _lib7_netdb_get_network_by_name   (Task* task,  Val arg)   {
    //===============================
    //
    // Mythryl type: String -> Null_Or(   (String, List(String), Addr_Family, Sysword)   )
    //
    // This fn gets bound as   get_network_by_name'   in:
    //
    //     src/lib/std/src/socket/net-db.pkg

    #if defined(OPSYS_WIN32)
        // XXX BUGGO FIXME:  getnetbyname() does not seem to exist under Windows.  What is the equivalent?
        return RAISE_ERROR(task, "<getnetbyname not implemented>");
    #else
	return _util_NetDB_mknetent (task, getnetbyname (HEAP_STRING_AS_C_STRING(arg)));
    #endif
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

