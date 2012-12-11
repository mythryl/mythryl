// get-signal-state.c
//
// This gets bound in:
//
//     src/lib/std/src/nj/runtime-signals-guts.pkg


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "system-dependent-signal-stuff.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/signal/cfun-list.h
// and thence
//     src/c/lib/signal/libmythryl-signal.c
// to
//     src/lib/std/src/nj/runtime-signals-guts.pkg

Val   _lib7_Sig_get_signal_state   (Task* task,  Val arg)   {
    //==========================
    //
    // Mythryl type:  Int -> Int
    //
    // This fn gets bound as   get_signal_state   in:
    //
    //     src/lib/std/src/nj/runtime-signals-guts.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int signal = TAGGED_INT_TO_C_INT( arg );

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	int state = get_signal_state (task->hostthread, signal );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    Val result = TAGGED_INT_FROM_C_INT(state);

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

