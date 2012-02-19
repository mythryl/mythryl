// tcsendbreak.c


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

//     SYNOPSIS
//            #include <termios.h>
//            #include <unistd.h>
//     
//            int tcsendbreak(int fd, int duration);


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

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_TTY_tcsendbreak");

    int fd       =  GET_TUPLE_SLOT_AS_INT( arg, 0 );
    int duration =  GET_TUPLE_SLOT_AS_INT( arg, 1 );

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_TTY_tcsendbreak", NULL );
	//
	int status = tcsendbreak( fd, duration );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_TTY_tcsendbreak" );

    RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

