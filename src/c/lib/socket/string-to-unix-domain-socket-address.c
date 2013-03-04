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
    //     src/lib/std/src/socket/unix-domain-socket--premicrothread.pkg

										ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
										ramlog_printf("#%d  %s/TOP, arg x=%08x\n",syscalls_seen,__func__, arg);

    char* path = HEAP_STRING_AS_C_STRING( arg );				// Last use of 'arg'.
										ramlog_printf("#%d  %s/AAA, path s='%s'\n",syscalls_seen,__func__, path);

    struct sockaddr_un	addr;
    memset(            &addr, 0, sizeof(struct sockaddr_un));
										ramlog_printf("#%d  %s/BBB, &addr p=%p   sizeof(struct sockaddr_un)   d=%d\n",syscalls_seen,__func__, &addr,sizeof(struct sockaddr_un));

    addr.sun_family = AF_UNIX;

										ramlog_printf("#%d  %s/CCC, &addr p=%p   sizeof(add.sun_path) d=%d   strlen(path) d=%d\n",syscalls_seen,__func__, &addr,sizeof(addr.sun_path),strlen(path));
    strcpy( addr.sun_path, path );

    int len;
    //
    #ifdef SOCKADDR_HAS_LEN
	len = strlen(path)
            + sizeof(addr.sun_len)
            + sizeof(addr.sun_family)
            + 1;
	addr.sun_len = len;
										ramlog_printf("#%d  %s/DDD len d=%d\n",syscalls_seen,__func__, len);
    #else
	len = strlen(path)
            + sizeof(addr.sun_family)
            + 1;
										ramlog_printf("#%d  %s/EEE len d=%d\n",syscalls_seen,__func__, len);
    #endif

    Val data =  make_biwordslots_vector_sized_in_bytes__may_heapclean(task, &addr, len, NULL );
										ramlog_printf("#%d  %s/FFF data x=%08x\n",syscalls_seen,__func__, data);

    Val result = make_vector_header(task,  UNT8_RO_VECTOR_TAGWORD, data, len );

										EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.

