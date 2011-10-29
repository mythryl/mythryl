// tcsendbreak.c


#include "../../mythryl-config.h"

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



Val   _lib7_P_TTY_tcsendbreak   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:   (Int, Int) -> Void
    //
    // Send break condition on tty line.
    //
    // This fn gets bound as   tcsendbreak   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-tty.pkg

    int status = tcsendbreak(GET_TUPLE_SLOT_AS_INT(arg, 0),GET_TUPLE_SLOT_AS_INT(arg, 1));
    //
    CHECK_RETURN_UNIT(task, status)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

