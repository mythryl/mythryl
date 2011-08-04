// times.c



#include "../../config.h"

#include "system-dependent-unix-stuff.h"
#include <sys/times.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-procenv/cfun-list.h
// and thence
//     src/c/lib/posix-procenv/libmythryl-posix-process-environment.c



Val   _lib7_P_ProcEnv_times   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type:   Void -> (Int, Int, Int, Int, Int)
    //
    // Return process and child process times, in clock ticks.
    //
    // This fn gets bound as   times'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-id.pkg


    Val  v, e, u, s, cu, cs;

    struct tms   ts;
    clock_t t = times( &ts );

    if (t == -1)   return RAISE_SYSERR(task, -1);

    INT32_ALLOC(task, e, t);
    INT32_ALLOC(task, u, ts.tms_utime);
    INT32_ALLOC(task, s, ts.tms_stime);
    INT32_ALLOC(task, cu, ts.tms_cutime);
    INT32_ALLOC(task, cs, ts.tms_cstime);
    REC_ALLOC5(task, v, e, u, s, cu, cs);

    return v;
}

// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

