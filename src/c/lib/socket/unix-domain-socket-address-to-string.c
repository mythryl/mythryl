// unix-domain-socket-address-to-string.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"
#include "sockets-osdep.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include INCLUDE_SOCKET_H
#include INCLUDE_UN_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"


/*
###           "If builders built buildings the
###            way programmers write programs,
###            then the first woodpecker that came
###            along would destroy civilization."
###
###                       -- Gerald M Weinberg
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_unix_domain_socket_address_to_string   (Task* task,  Val arg)   {
    //===============================================
    //
    // Mythryl Type:   Internet_Address -> String
    //
    // Given a UNIX-domain socket address, return the string.
    //
    // This fn gets bound to   unix_domain_socket_address_to_string'   in:
    //
    //     src/lib/std/src/socket/unix-domain-socket--premicrothread.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    struct sockaddr_un*  addr
	=
	GET_VECTOR_DATACHUNK_AS( struct sockaddr_un*, arg );

    ASSERT( addr->sun_family == AF_UNIX );

    Val result =  make_ascii_string_from_c_string__may_heapclean( task, addr->sun_path, NULL );		// make_ascii_string_from_c_string__may_heapclean	def in    src/c/heapcleaner/make-strings-and-vectors-etc.c

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}



// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

