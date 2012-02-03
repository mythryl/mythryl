// fcntl_gfd.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "system-dependent-unix-stuff.h"

#if HAVE_FCNTL_H
    #include <fcntl.h>
#endif

#include "make-strings-and-vectors-etc.h"
    #include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-io/cfun-list.h
// and thence
//     src/c/lib/posix-io/libmythryl-posix-io.c



Val   _lib7_P_IO_fcntl_gfd   (Task* task,  Val arg)   {
    //====================
    //
    // Mythryl type:   Int -> Unt
    //
    // Get the close-on-exec flag associated with the file descriptor.
    //
    // This fn gets bound as   fcntl_gfd   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-io.pkg
    //     src/lib/std/src/posix-1003.1b/posix-io-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_IO_fcntl_gfd");

    int flag;

/*  do { */						// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_IO_fcntl_gfd", arg );
	    //
	    flag = fcntl(TAGGED_INT_TO_C_INT(arg), F_GETFD);
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_IO_fcntl_gfd" );

/*  } while (flag < 0 && errno == EINTR);	*/	// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    if (flag == -1) {
        return RAISE_SYSERR(task, flag);
    } else {
        return  make_one_word_unt(task, flag);
    }
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

