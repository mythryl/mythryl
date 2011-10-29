// listsignals.c
//
// This gets bound in:
//
//     src/lib/std/src/nj/runtime-signals-guts.pkg


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "system-dependent-signal-stuff.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/signal/cfun-list.h
// and thence
//     src/c/lib/signal/libmythryl-signal.c



Val   _lib7_Sig_listsigs   (Task* task,  Val arg)   {
    //==================
    //
    // List the supported signals.
    //
    // Mythryl type:  Void -> List(System_Constant)
    //
    // This gets bound as   list_signals'   in:
    //
    //     src/lib/std/src/nj/runtime-signals-guts.pkg


    return   list_signals( task );		// See src/c/machine-dependent/posix-signal.c
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

