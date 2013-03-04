// pause.c


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

#include <stdio.h>
#include <string.h>

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif



// One of the library bindings exported via
//     src/c/lib/posix-process/cfun-list.h
// and thence
//     src/c/lib/posix-process/libmythryl-posix-process.c



Val   _lib7_P_Process_pause   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type: Void -> Void
    //
    // Wait for a POSIX interprocess signal.
    //
    // This fn gets bound as   pause   in:
    //
    //     src/lib/std/src/psx/posix-process.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	pause ();								// Documentation in	man 2 pause
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return HEAP_VOID;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.

