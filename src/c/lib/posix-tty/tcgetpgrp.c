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
    //     src/lib/std/src/psx/posix-tty.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int fd = TAGGED_INT_TO_C_INT( arg );

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	int iresult = tcgetpgrp( fd );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    Val result = TAGGED_INT_FROM_C_INT( iresult );

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

