// dup2.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif


// One of the library bindings exported via
//     src/c/lib/posix-io/cfun-list.h
// and thence
//     src/c/lib/posix-io/libmythryl-posix-io.c



Val   _lib7_P_IO_dup2   (Task* task,  Val arg)   {
    //===============
    //
    // Mythryl type:   (Int, Int) -> Void
    //
    // Duplicate an open file descriptor
    //
    // This fn gets bound as   dup2'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-io.pkg
    //     src/lib/std/src/posix-1003.1b/posix-io-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_IO_dup2");

    int fd0 =  GET_TUPLE_SLOT_AS_INT( arg, 0 );
    int fd1 =  GET_TUPLE_SLOT_AS_INT( arg, 1 );

    int status;

    do {						// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c
							// Restored   2012-09-03 CrT

	RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_IO_dup2", NULL );
	    //
	    status = dup2( fd0, fd1 );
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_IO_dup2" );

// if (errno == EINTR) puts("Error: EINTR in dup2.c\n");
    } while (status < 0 && errno == EINTR);		// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

