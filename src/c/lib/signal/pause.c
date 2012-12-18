// pause.c
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

/*
###   "More than iron, more than lead, more than gold, I need electricity.
###    I need it more than I need lamb or pork or lettuce or cucumber.
###    I need it for my dreams."
###
###            -- Racter (a program that sometimes writes poetry)
*/



// One of the library bindings exported via
//     src/c/lib/signal/cfun-list.h
// and thence
//     src/c/lib/signal/libmythryl-signal.c



Val   _lib7_Sig_pause   (Task* task,  Val arg)   {
    //===============
    //
    // Mythryl type:   Void -> Void
    //
    // Pause until the next signal.
    //
    // This fn gets bound as   pause   in:
    //
    //     src/lib/std/src/nj/interprocess-signals-guts.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	pause_until_signal( task->hostthread );			//  pause_until_signal	def in   src/c/machine-dependent/interprocess-signals.c
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );


									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return HEAP_VOID;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

