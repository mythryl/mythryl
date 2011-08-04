// tcdrain.c


#include "../../config.h"

#include "system-dependent-unix-stuff.h"

#if HAVE_TERMIOS_H
    #include <termios.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
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
    //     src/lib/std/src/posix-1003.1b/posix-tty.pkg


    int fd     =  INT31_TO_C_INT( arg );
    int status =  tcdrain( fd );

    CHECK_RETURN_UNIT(task, status)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

