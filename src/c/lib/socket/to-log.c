// to-log.c


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


// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c

Val   _lib7_Sock_to_log   (Task* task,  Val arg)   {
    //===============================================
    //
    // Mythryl type:   String -> Void
    //
    // Write string to currently open logfile via log_if from
    //
    //     src/c/main/error-reporting.c

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Sock_to_log");

    char* string = HEAP_STRING_AS_C_STRING( arg );

    log_if ("%s", string);				// Safer than doing just log_if(string) -- the string might have a '%' in it.

    return HEAP_VOID;
}

// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

