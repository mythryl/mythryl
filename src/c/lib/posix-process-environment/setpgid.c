// setpgid.c



#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
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



Val   _lib7_P_ProcEnv_setpgid   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:  (Int, Int) -> Void
    //
    // Set user id
    //
    // This fn gets bound as   set_process_group_id   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-id.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_ProcEnv_setpgid");

    int pid  =  GET_TUPLE_SLOT_AS_INT(arg,0);
    int pgid =  GET_TUPLE_SLOT_AS_INT(arg,1);

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_setpgid", arg );
	//
	int status =  setpgid( pid, pgid );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_setpgid" );

    CHECK_RETURN_UNIT( task, status )
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

