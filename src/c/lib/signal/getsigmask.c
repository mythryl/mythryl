// getsigmask.c


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "system-dependent-signal-stuff.h"
#include "cfun-proto-list.h"


// One of the library bindings exported via
//     src/c/lib/signal/cfun-list.h
// and thence
//     src/c/lib/signal/libmythryl-signal.c



Val   _lib7_Sig_getsigmask   (Task* task,  Val arg) {
    //====================
    //
    // Mythryl type:   Void -> Null_Or( List( System_Constant ) )
    //
    // This gets bound as   get_sig_mask   in:
    //
    //     src/lib/std/src/nj/runtime-signals-guts.pkg
    //

										ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Val result = get_signal_mask__may_heapclean( task, arg, NULL );					// See, e.g., src/c/machine-dependent/posix-signal.c

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

