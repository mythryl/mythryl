// set-signal-state.c
//
// This gets bound in:
//
//     src/lib/std/src/nj/interprocess-signals-guts.pkg


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



Val   _lib7_Sig_set_signal_state   (Task* task,  Val arg)   {
    //==========================
    //
    // Mythryl type: (Int /*signal*/, Int /*state*/) -> Void
    //
    // This fn gets bound as  set_signal_state   in:
    //
    //     src/lib/std/src/nj/interprocess-signals-guts.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int signal_number =  GET_TUPLE_SLOT_AS_INT( arg, 0 );
    int signal_state  =  GET_TUPLE_SLOT_AS_INT( arg, 1 );				// Last use of 'arg'.

    RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_Sig_setsigstate", NULL );
	//
	set_signal_state(								// set_signal_state	def in    src/c/machine-dependent/posix-signal.c
	    //
	    task->hostthread,
	    signal_number,
	    signal_state
	);
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_Sig_setsigstate" );

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return HEAP_VOID;
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

