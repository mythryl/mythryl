// ascii-name-to-portable-signal-id.c
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
// to
//     src/lib/std/src/nj/interprocess-signals-guts.pkg

Val   _lib7_Sig_ascii_signal_name_to_portable_signal_id   (Task* task,  Val arg)   {
    //=================================================
    //
    // Mythryl type:  String -> Int
    //
    // This fn gets bound as   ascii_signal_name_to_portable_signal_id   in:
    //
    //     src/lib/std/src/nj/interprocess-signals-guts.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    char* signal_name = HEAP_STRING_AS_C_STRING( arg );

	//
    int signal_id = ascii_signal_name_to_portable_signal_id ( signal_name );		// ascii_name_to_portable_signal_id	is from   src/c/machine-dependent/interprocess-signals.c
	//

    Val result = TAGGED_INT_FROM_C_INT( signal_id );

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.

