// string-to-unix-domain-socket-address.c


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
#include "raise-error.h"
#include "cfun-proto-list.h"



Val   _lib7_Sock_string_to_unix_domain_socket_address   (Task* task,  Val arg)   {
    //===============================================
    //
    // Mythryl type:   String -> Internet_Address
    //
    // Given a path, allocate a UNIX-domain socket address.
    //
    // This fn gets bound to   string_to_unix_domain_socket_address'   in:
    //
    //     src/lib/std/src/socket/unix-domain-socket.pkg

										ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Sock_string_to_unix_domain_socket_address");

    char* path = HEAP_STRING_AS_C_STRING( arg );				// Last use of 'arg'.

    struct sockaddr_un	addr;
    memset(            &addr, 0, sizeof(struct sockaddr_un));

    addr.sun_family = AF_UNIX;

    strcpy( addr.sun_path, path );

    int len;
    //
    #ifdef SOCKADDR_HAS_LEN
	len = strlen(path)
            + sizeof(addr.sun_len)
            + sizeof(addr.sun_family)
            + 1;
	addr.sun_len = len;
    #else
	len = strlen(path)
            + sizeof(addr.sun_family)
            + 1;
    #endif

    Val data =  make_biwordslots_vector_sized_in_bytes__may_heapclean(task, &addr, len, NULL );

    return  make_vector_header(task,  UNT8_RO_VECTOR_TAGWORD, data, len );
}

// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

