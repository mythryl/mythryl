// setsigstate.c
//
// This gets bound in:
//
//     src/lib/std/src/nj/runtime-signals-guts.pkg


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "task.h"
#include "system-dependent-signal-stuff.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/signal/cfun-list.h
// and thence
//     src/c/lib/signal/libmythryl-signal.c



Val   _lib7_Sig_setsigstate   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type: (System_Constant, Int) -> Void
    //
    // This fn gets bound as  set_signal_state   in:
    //
    //     src/lib/std/src/nj/runtime-signals-guts.pkg


    Val	sig = GET_TUPLE_SLOT_AS_VAL(arg, 0);
    //
    set_signal_state(								// set_signal_state	def in    src/c/machine-dependent/posix-signal.c
	//
        task->pthread,
	GET_TUPLE_SLOT_AS_INT(sig, 0),
        GET_TUPLE_SLOT_AS_INT(arg, 1)
    );
    //
    return HEAP_VOID;
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

