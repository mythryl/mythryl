// readbuf.c


#include "../../config.h"

#include <errno.h>

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-io/cfun-list.h
// and thence
//     src/c/lib/posix-io/libmythryl-posix-io.c



Val   _lib7_P_IO_readbuf   (Task* task,  Val arg)   {
    //==================
    //
    // Mythryl type: (Int, rw_unt8_vector::Rw_Vector, Int,   Int) -> Int
    //                fd   data                       nbytes start
    //
    // Read nbytes of data from the specified file
    // into the given array starting at start.
    // Return the number of bytes read.
    // Assume bounds have been checked.
    //
    // This fn gets bound as   readbuf'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-io.pkg
    //     src/lib/std/src/posix-1003.1b/posix-io-64.pkg

    int	  fd     =                                GET_TUPLE_SLOT_AS_INT( arg, 0 );
    Val	  buf    =                                GET_TUPLE_SLOT_AS_VAL( arg, 1 );
    int	  nbytes =                                GET_TUPLE_SLOT_AS_INT( arg, 2 );
    char* start  = HEAP_STRING_AS_C_STRING(buf) + GET_TUPLE_SLOT_AS_INT( arg, 3 );

    int  n;

/*  do { */					// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

        n = read (fd, start, nbytes);

/*  } while (n < 0 && errno == EINTR);	*/	// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    CHECK_RETURN (task, n)
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

