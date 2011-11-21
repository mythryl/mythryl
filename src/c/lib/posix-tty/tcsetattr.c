// tcsetattr.c


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
#include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-tty/cfun-list.h
// and thence
//     src/c/lib/posix-tty/libmythryl-posix-tty.c



Val   _lib7_P_TTY_tcsetattr   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type:   (Int, Int, Termio_Rep -> Void)
    // where
    //    Termio_Rep = (Unt, Unt, Unt, Unt, String, Unt, Unt)
    //
    // Set parameters associated with tty.
    //
    // NOTE: the calls to cfset[io]speed by making the code more OS-dependent
    // and using the package of struct termios.
    //
    // This fn gets bound as   tcsetattr   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-tty.pkg
    
    int fd         =  GET_TUPLE_SLOT_AS_INT(arg, 0);
    int action     =  GET_TUPLE_SLOT_AS_INT(arg, 1);
    Val termio_rep =  GET_TUPLE_SLOT_AS_VAL(arg, 2);

    struct termios   data;

    data.c_iflag = TUPLE_GETWORD(         termio_rep, 0);
    data.c_oflag = TUPLE_GETWORD(         termio_rep, 1);
    data.c_cflag = TUPLE_GETWORD(         termio_rep, 2);
    data.c_lflag = TUPLE_GETWORD(         termio_rep, 3);
    Val c_cc     = GET_TUPLE_SLOT_AS_VAL( termio_rep, 4);
    int ispeed   = TUPLE_GETWORD(         termio_rep, 5);
    int ospeed   = TUPLE_GETWORD(         termio_rep, 6);

    memcpy (data.c_cc, GET_VECTOR_DATACHUNK_AS( void*, c_cc ), NCCS);


    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_TTY_tcsetattr", arg );
	//
	int status = cfsetispeed (&data, ispeed);
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_TTY_tcsetattr" );


    if (status < 0)   return RAISE_SYSERR(task, status);


    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_TTY_tcsetattr", arg );
	//
	status = cfsetospeed (&data, ospeed);
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_TTY_tcsetattr" );


    if (status < 0)   return RAISE_SYSERR(task, status);


    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_TTY_tcsetattr", arg );
	//
	status = tcsetattr(fd, action, &data);
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_TTY_tcsetattr" );


    CHECK_RETURN_UNIT( task, status )
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

