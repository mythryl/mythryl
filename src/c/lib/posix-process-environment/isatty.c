// isatty.c



#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "cfun-proto-list.h"

#include <stdio.h>
#include <string.h>

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif



// One of the library bindings exported via
//     src/c/lib/posix-process-environment/cfun-list.h
// and thence
//     src/c/lib/posix-process-environment/libmythryl-posix-process-environment.c



Val   _lib7_P_ProcEnv_isatty   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type:  Int -> Bool
    //
    // Is file descriptor associated with a terminal device?
    //
    // This fn gets bound as   isatty'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-id.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_ProcEnv_isatty");

    RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_ProcEnv_isatty", NULL );
	//
	int result = isatty(TAGGED_INT_TO_C_INT(arg));
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_ProcEnv_isatty" );

    return (result ? HEAP_TRUE : HEAP_FALSE);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

