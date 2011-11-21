// getsigstate.c
//
// This gets bound in:
//
//     src/lib/std/src/nj/runtime-signals-guts.pkg


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "task.h"
#include "system-dependent-signal-stuff.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/signal/cfun-list.h
// and thence
//     src/c/lib/signal/libmythryl-signal.c



Val   _lib7_Sig_getsigstate   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type:  System_Constant -> Int
    //
    // This fn gets bound as   get_signal_state   in:
    //
    //     src/lib/std/src/nj/runtime-signals-guts.pkg

    int sig_num = GET_TUPLE_SLOT_AS_INT(arg, 0);

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_Sig_getsigstate", arg );
	//
	int state = get_signal_state (task->pthread, sig_num );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_Sig_getsigstate" );

    return TAGGED_INT_FROM_C_INT(state);
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

