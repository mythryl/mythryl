// time.c


#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include <time.h>



// One of the library bindings exported via
//     src/c/lib/posix-procenv/cfun-list.h
// and thence
//     src/c/lib/posix-procenv/libmythryl-posix-process-environment.c



Val   _lib7_P_ProcEnv_time   (Task* task,  Val arg)   {
    //====================
    //
    // Mythryl type:  Void -> int1::Int
    //
    // Return time in seconds from 00:00:00 UTC, January 1, 1970
    //
    // This fn gets bound as   time   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-id.pkg

    time_t t =  time( NULL );
    //
    Val	              result;
    INT1_ALLOC(task, result, t);
    return            result;
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

