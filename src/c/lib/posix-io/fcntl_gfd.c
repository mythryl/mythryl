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
    #include "raise-error.h"
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
    //     src/lib/std/src/psx/posix-io.pkg
    //     src/lib/std/src/psx/posix-io-64.pkg

										ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int flag;

    do {
	RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	    //
	    flag = fcntl(TAGGED_INT_TO_C_INT(arg), F_GETFD);
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );
	//
    } while (flag < 0 && errno == EINTR);					// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    Val result;

    if (flag == -1)	result = RAISE_SYSERR__MAY_HEAPCLEAN(task, flag, NULL);
    else		result = make_one_word_unt(task, flag);

										EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

