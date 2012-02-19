// ttyname.c



#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
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



Val   _lib7_P_ProcEnv_ttyname   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:   Int -> String
    //
    // Return terminal name associated with file descriptor, if any.
    //
    // This fn gets bound as   ttyname'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-id.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_ProcEnv_ttyname");

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_ttyname", NULL );
	//
	char* name = ttyname(TAGGED_INT_TO_C_INT(arg));
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_ttyname" );

    if (name == NULL)   return RAISE_ERROR__MAY_HEAPCLEAN(task, "not a terminal device", NULL);
    //  
    return make_ascii_string_from_c_string__may_heapclean( task, name, NULL );
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

