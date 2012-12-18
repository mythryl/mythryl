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
    //     src/lib/std/src/psx/posix-id.pkg

											ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	char* name = ttyname(TAGGED_INT_TO_C_INT(arg));
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    if (name == NULL)   return RAISE_ERROR__MAY_HEAPCLEAN(task, "not a terminal device", NULL);
    //  
    Val result = make_ascii_string_from_c_string__may_heapclean( task, name, NULL );
											EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

