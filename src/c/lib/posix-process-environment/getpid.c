// getpid.c


#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif


// One of the library bindings exported via
//     src/c/lib/posix-process-environment/cfun-list.h
// and thence
//     src/c/lib/posix-process-environment/libmythryl-posix-process-environment.c



Val   _lib7_P_ProcEnv_getpid   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type:   Void -> Int
    //
    // Return the process id of the current process.
    //
    // This fn gets bound as   get_process_id   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-id.pkg

    return TAGGED_INT_FROM_C_INT( getpid() );
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

