// fork.c


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif



// One of the library bindings exported via
//     src/c/lib/posix-process/cfun-list.h
// and thence
//     src/c/lib/posix-process/libmythryl-posix-process.c



Val   _lib7_P_Process_fork   (Task* task,  Val arg)   {
    //====================
    //
    // Mythryl type:   Void -> Int
    //
    // Fork a new process.
    //
    // This fn gets bound as   fork'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-process.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_Process_fork");

    int status = fork ();
    //
    RETURN_STATUS_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

