// setgid.c



#include "../../mythryl-config.h"

#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
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



Val   _lib7_P_ProcEnv_setgid   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type:  Unt -> Void
    //
    // Set group id.
    //
    // This fn gets bound as   set_group_id   in:
    //
    //     src/lib/std/src/psx/posix-id.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_ProcEnv_setgid");

    RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_ProcEnv_setgid", &arg );
	//
	int status =  setgid( WORD_LIB7toC( arg ));
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_ProcEnv_setgid" );

    RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

