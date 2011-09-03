// fcntl_gfd.c


#include "../../config.h"

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

    int flag;
    Val v;

/*  do { */						// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

        flag = fcntl(TAGGED_INT_TO_C_INT(arg), F_GETFD);

/*  } while (flag < 0 && errno == EINTR);	*/	// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    if (flag == -1) {
        return RAISE_SYSERR(task, flag);
    } else {
        WORD_ALLOC (task, v, flag);
        return v;
    }
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

