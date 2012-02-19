// getuid.c



#include "../../mythryl-config.h"

#include "make-strings-and-vectors-etc.h"
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



Val   _lib7_P_ProcEnv_getuid   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type:   Void -> Unt
    //
    // Return user id.
    //
    // This fn gets bound as   get_user_id   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-id.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_ProcEnv_getuid");

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_getuid", &arg );
	//
	int uid = getuid ();
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_getuid" );

    return  make_one_word_unt(task,  (Val_Sized_Unt) uid  );
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

