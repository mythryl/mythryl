// fcntl_sfd.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "system-dependent-unix-stuff.h"

#if HAVE_FCNTL_H
    #include <fcntl.h>
#endif

#include "make-strings-and-vectors-etc.h"
    #include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-io/cfun-list.h
// and thence
//     src/c/lib/posix-io/libmythryl-posix-io.c



Val   _lib7_P_IO_fcntl_sfd   (Task* task,  Val arg)   {
    //====================
    //
    // Mythryl type:  (Int, Unt) -> Void
    //
    // Set the close-on-exec flag associated with the file descriptor.
    //
    // This fn gets bound as   fcntl_sfd   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-io.pkg
    //     src/lib/std/src/posix-1003.1b/posix-io-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_IO_fcntl_sfd");

    int             status;

    int             fd0  = GET_TUPLE_SLOT_AS_INT( arg, 0 );
    Vunt   flag = TUPLE_GETWORD(         arg, 1 );


    do {						// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c
							// Restored   2012-09-03 CrT
	RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_IO_fcntl_sfd", NULL );
	    //
	    status = fcntl(fd0, F_SETFD, flag);
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_IO_fcntl_sfd" );

// if (errno == EINTR) puts("Error: EINTR in fcntl_sfd.c\n");
    } while (status < 0 && errno == EINTR);		// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

