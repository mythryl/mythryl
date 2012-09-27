// tcdrain.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#include <stdio.h>
#include <string.h>

#if HAVE_TERMIOS_H
    #include <termios.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-tty/cfun-list.h
// and thence
//     src/c/lib/posix-tty/libmythryl-posix-tty.c



Val   _lib7_P_TTY_tcdrain   (Task* task,  Val arg)   {
    //===================
    //
    // Mythryl type:   Int -> Void
    //
    // Wait for all output to be transmitted.
    //
    // This fn gets bound as   tcdrain   in:
    //
    //     src/lib/std/src/psx/posix-tty.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_TTY_tcdrain");

    int fd     =  TAGGED_INT_TO_C_INT( arg );

    RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_TTY_tcdrain", NULL );
	//
	int status =  tcdrain( fd );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_TTY_tcdrain" );

    RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

