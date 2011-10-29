// interval-tick.c

#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

// One of the library bindings exported via
//     src/c/lib/heap/cfun-list.h
// and thence
//     src/c/lib/heap/libmythryl-heap.c



Val   _lib7_runtime_interval_tick__unimplemented   (Task* task,  Val arg)   {
    //===========================
    //
    // Mythryl type: : Void -> (Int, Int)
    //
    // This fn gets bound as   tick'   in:
    //
    //     src/lib/std/src/nj/set-sigalrm-frequency.pkg

    return RAISE_ERROR( task, "interval_tick unimplemented");
}



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

