// tcflush.c


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
//            int tcflush(int fd, int queue_selector);


Val    _lib7_P_TTY_tcflush   (Task* task,  Val arg)   {
    //====================
    //
    // Mythryl type:  (Int, Int) -> Void
    //
    // Discard data that is written but not sent, or received but not read.
    //
    // This fn gets bound as   tcflush   in:
    //
    //     src/lib/std/src/psx/posix-tty.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int fd     =  GET_TUPLE_SLOT_AS_INT( arg, 0 );
    int queue  =  GET_TUPLE_SLOT_AS_INT( arg, 1 );

    RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_TTY_tcflush", NULL );
	//
	int status =  tcflush( fd, queue );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_TTY_tcflush" );

    Val result =  RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

