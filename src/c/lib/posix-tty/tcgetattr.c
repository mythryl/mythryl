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

    iflag  =  make_one_word_unt(task, data.c_iflag  );
    oflag  =  make_one_word_unt(task, data.c_oflag  );
    cflag  =  make_one_word_unt(task, data.c_cflag  );
    lflag  =  make_one_word_unt(task, data.c_lflag  );

    ispeed =  make_one_word_unt(task, cfgetispeed (&data) );
    ospeed =  make_one_word_unt(task, cfgetospeed (&data) );
    
    // Allocate the vector.
    // Note that this might trigger a cleaning:
    //
    cc = allocate_nonempty_ascii_string__may_heapclean (task, NCCS);

    memcpy(
	GET_VECTOR_DATACHUNK_AS( void*, cc ),
        data.c_cc,
	NCCS
    );

    set_slot_in_nascent_heapchunk   (task, 0, MAKE_TAGWORD(PAIRS_AND_RECORDS_BTAG, 7));
    set_slot_in_nascent_heapchunk   (task, 1, iflag);
    set_slot_in_nascent_heapchunk   (task, 2, oflag);
    set_slot_in_nascent_heapchunk   (task, 3, cflag);
    set_slot_in_nascent_heapchunk   (task, 4, lflag);
    set_slot_in_nascent_heapchunk   (task, 5, cc);
    set_slot_in_nascent_heapchunk   (task, 6, ispeed);
    set_slot_in_nascent_heapchunk   (task, 7, ospeed);

    return commit_nascent_heapchunk (task, 7);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

