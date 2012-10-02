// tcflow.c


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
//            int tcflow(int fd, int action);


Val   _lib7_P_TTY_tcflow   (Task* task,  Val arg)   {
    //==================
    //
    // Mythrryl type:   (Int, Int) -> Void
    //
    // Suspend transmission or receipt of data.
    //
    // This fn gets bound as   tcflow   in:
    //
    //     src/lib/std/src/psx/posix-tty.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_TTY_tcflow");

    int fd     =  GET_TUPLE_SLOT_AS_INT( arg, 0 );
    int action =  GET_TUPLE_SLOT_AS_INT( arg, 1 );

    RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_TTY_tcflow", NULL );
	//
	int status =  tcflow( fd, action );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_TTY_tcflow" );

    return  RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

