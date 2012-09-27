// setsid.c



#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-process-environment/cfun-list.h
// and thence
//     src/c/lib/posix-process-environment/libmythryl-posix-process-environment.c



Val   _lib7_P_ProcEnv_setsid   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type:   Void -> Int
    //
    // Set session id
    //
    // This fn gets bound as   set_session_id   in:
    //
    //     src/lib/std/src/psx/posix-id.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_ProcEnv_setsid");

    RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_ProcEnv_setsid", NULL );
	//
	pid_t pid =  setsid ();
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_ProcEnv_setsid" );

    RETURN_STATUS_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, pid, NULL);
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

