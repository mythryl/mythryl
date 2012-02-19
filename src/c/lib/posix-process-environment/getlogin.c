// getlogin.c



#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif



// One of the library bindings exported via
//     src/c/lib/posix-process-environment/cfun-list.h
// and thence
//     src/c/lib/posix-process-environment/libmythryl-posix-process-environment.c



Val   _lib7_P_ProcEnv_getlogin   (Task* task,  Val arg)   {
    //========================
    //
    // Mythryl type:  Void -> String
    //
    // Return login name
    //
    // This fn gets bound as   get_login   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-id.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_ProcEnv_getlogin");

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_getlogin", &arg );
	//
	char* name = getlogin ();
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_getlogin" );

    if (name == NULL)   return RAISE_ERROR__MAY_HEAPCLEAN(task, "no login name", NULL);
  
    return  make_ascii_string_from_c_string__may_heapclean( task, name, NULL );
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

