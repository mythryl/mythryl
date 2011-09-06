// write.c


#include "../../config.h"

#include <errno.h>

#include "system-dependent-unix-stuff.h"

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



Val   _lib7_P_IO_write   (Task* task,  Val arg)   {
    //================
    //
    // Mythryl type:  (Int, vector_of_one_byte_unts::Vector, Int) -> Int
    //
    // Write the number of bytes of data from the given vector,
    // starting at index 0, to the specified file.  Return the
    // number of bytes written. Assume bounds checks have been done.
    //
    // One would expect this fn to be bound as   write'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-io.pkg
    //     src/lib/std/src/posix-1003.1b/posix-io-64.pkg
    //
    // but in fact it appears to be nowhere referenced. (!) Should be called or deleted. XXX BUGGO FIXME

    int		fd     = GET_TUPLE_SLOT_AS_INT( arg, 0 );
    Val		data   = GET_TUPLE_SLOT_AS_VAL( arg, 1 );
    size_t	nbytes = GET_TUPLE_SLOT_AS_INT( arg, 2 );

    ssize_t    	n;

/*  do { */					// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

        n = write (fd, HEAP_STRING_AS_C_STRING(data), nbytes);

/*  } while (n < 0 && errno == EINTR);	*/	// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    CHECK_RETURN (task, n)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

