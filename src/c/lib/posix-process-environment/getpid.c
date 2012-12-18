// getpid.c


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
//
// We get bound as  get_process_id  in   src/lib/std/src/psx/posix-id.pkg


Val   _lib7_P_ProcEnv_getpid   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type:   Void -> Int
    //
    // Return the process id of the current process.
    //
    // This fn gets bound as   get_process_id   in:
    //
    //     src/lib/std/src/psx/posix-id.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	int pid = getpid();
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    Val result = TAGGED_INT_FROM_C_INT( pid );
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

