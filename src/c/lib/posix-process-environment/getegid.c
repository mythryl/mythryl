// getegid.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"


// One of the library bindings exported via
//     src/c/lib/posix-process-environment/cfun-list.h
// and thence
//     src/c/lib/posix-process-environment/libmythryl-posix-process-environment.c



Val   _lib7_P_ProcEnv_getegid   (Task* task,  Val arg)   {
    //=======================
    //
    // _lib7_P_ProcEnv_getegid: Void -> word
    //
    // Return effective group id
    //
    // This fn gets bound as   get_effective_group_id   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-id.pkg

    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_ProcEnv_getegid");

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_getegid", arg );
	//
	int egid = getegid();
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_getegid" );

    Val	              result;
    WORD_ALLOC (task, result, (Val_Sized_Unt)egid);
    return            result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

