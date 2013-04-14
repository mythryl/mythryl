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

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	pid_t pid =  setsid ();
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    Val result =  RETURN_STATUS_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, pid, NULL);

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

