// signal-is-supported-by-host-os.c
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



Val   _lib7_Sig_signal_is_supported_by_host_os   (Task* task,  Val arg)   {
    //========================================
    //
    // Mythryl type: Int /*signal*/ -> Void
    //
    // This fn gets bound as  signal_is_supported_by_host_os   in:
    //
    //     src/lib/std/src/nj/interprocess-signals-guts.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int signal = TAGGED_INT_TO_C_INT( arg );						// Last use of 'arg'.

    int result = portable_signal_id_to_host_os_signal_id( signal );

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return  result ? HEAP_TRUE : HEAP_FALSE;
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

