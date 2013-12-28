// tcsetpgrp.c


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


//     NAME
//            tcgetpgrp, tcsetpgrp - get and set terminal foreground process group
//     
//     SYNOPSIS
//            #include <unistd.h>
//     
//            int tcsetpgrp(int fd, pid_t pgrp);


Val   _lib7_P_TTY_tcsetpgrp   (Task* task,  Val arg)   {
    //=====================
    //
    // _lib7_P_TTY_tcsetpgrp : (Int, Int) -> Void
    //
    // Set foreground process group id of tty.
    //
    // This fn gets bound as   tcsetpgrp   in:
    //
    //     src/lib/std/src/psx/posix-tty.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int fd     = GET_TUPLE_SLOT_AS_INT(arg, 0);
    int pgrp   = GET_TUPLE_SLOT_AS_INT(arg, 1);

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	int status = tcsetpgrp( fd, pgrp );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    Val result =  RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

