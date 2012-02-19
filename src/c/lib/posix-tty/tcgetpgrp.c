// tcgetpgrp.c


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



Val   _lib7_P_TTY_tcgetpgrp   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type:   Int -> Int
    //
    // Get foreground process group id of tty.
    //
    // This fn gets bound as   tcgetpgrp   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-tty.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_TTY_tcgetpgrp");

    int fd = TAGGED_INT_TO_C_INT( arg );

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_TTY_tcgetpgrp", NULL );
	//
	int result = tcgetpgrp( fd );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_TTY_tcgetpgrp" );

    return TAGGED_INT_FROM_C_INT( result );
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

