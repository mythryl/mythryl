// tcgetattr.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"
#include INCLUDE_TIME_H

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



Val   _lib7_P_TTY_tcgetattr   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type:   Int -> (Unt, Unt, Unt, Unt, String, Unt, Unt)
    //
    // Get parameters associated with tty.
    //
    // NOTE: the calls to cfget[io] speed by making the code more OS-dependent
    // and using the package of struct termios.
    //
    // This fn gets bound as   tcgetattr   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-tty.pkg

    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_TTY_tcgetattr");

    int fd = TAGGED_INT_TO_C_INT( arg );

    Val      iflag, oflag, cflag, lflag;
    Val      cc, ispeed, ospeed;

    struct termios  data;

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_TTY_tcgetattr", arg );
	//
	int status =  tcgetattr( fd, &data );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_TTY_tcgetattr" );

    if (status < 0)   return RAISE_SYSERR(task, status);

    WORD_ALLOC (task, iflag, data.c_iflag);
    WORD_ALLOC (task, oflag, data.c_oflag);
    WORD_ALLOC (task, cflag, data.c_cflag);
    WORD_ALLOC (task, lflag, data.c_lflag);
    WORD_ALLOC (task, ispeed, cfgetispeed (&data));
    WORD_ALLOC (task, ospeed, cfgetospeed (&data));
    
    // Allocate the vector.
    // Note that this might trigger a cleaning:
    //
    cc = allocate_nonempty_ascii_string (task, NCCS);

    memcpy(
	GET_VECTOR_DATACHUNK_AS( void*, cc ),
        data.c_cc,
	NCCS
    );

    LIB7_AllocWrite   (task, 0, MAKE_TAGWORD(PAIRS_AND_RECORDS_BTAG, 7));
    LIB7_AllocWrite   (task, 1, iflag);
    LIB7_AllocWrite   (task, 2, oflag);
    LIB7_AllocWrite   (task, 3, cflag);
    LIB7_AllocWrite   (task, 4, lflag);
    LIB7_AllocWrite   (task, 5, cc);
    LIB7_AllocWrite   (task, 6, ispeed);
    LIB7_AllocWrite   (task, 7, ospeed);

    return LIB7_Alloc (task, 7);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

