// exit.c

#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

/*
###     "We pray for one last landing
###        on the globe that gave us birth;
###      Let us rest our eyes on the fleecy skies
###        and the cool, green hills of Earth."
###
###                  -- "Noisy" Rhysling
*/

Val   _lib7_P_Process_exit   (Task* task,  Val arg)   {		//  : Int -> X
    //====================
    //
    // Exit from process
    //
    // This fn gets bound as   exit   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-process.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_Process_exit");

    print_stats_and_exit( TAGGED_INT_TO_C_INT( arg ) );				// Doesn't return.	def in   src/c/main/runtime-main.c

    exit(0);									// Cannot execute; just to suppress a gcc warning.
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

