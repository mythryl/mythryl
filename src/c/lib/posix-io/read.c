// read.c


#include "../../mythryl-config.h"

#include <errno.h>

#include "system-dependent-unix-stuff.h"

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



/*
###		"Read at every wait; read at all hours;
###              read within leisure; read in times of labor;
###              read as one goes in; read as one goes out.
###              The task of an educated mind is simply put:
###                 read to lead."
###
###                                      -- Cicero
*/



// One of the library bindings exported via
//     src/c/lib/posix-io/cfun-list.h
// and thence
//     src/c/lib/posix-io/libmythryl-posix-io.c



Val   _lib7_P_IO_read   (Task* task,  Val arg)   {
    //===============
    //
    // Mythryl type:   (Int, Int) -> vector_of_one_byte_unts::Vector
    //                  fd   nbytes
    //
    // Read the specified number of bytes from the specified file,
    // returning them in a vector.
    //
    // This fn gets bound as   read'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-io.pkg
    //     src/lib/std/src/posix-1003.1b/posix-io-64.pkg

    int fd     =  GET_TUPLE_SLOT_AS_INT(arg, 0);
    int nbytes =  GET_TUPLE_SLOT_AS_INT(arg, 1);

    if (nbytes == 0)   return ZERO_LENGTH_STRING_GLOBAL;

    // Allocate the vector.
    // Note that this might cause a cleaning, moving things around:
    //
    Val vec = allocate_nonempty_int1_vector( task, BYTES_TO_WORDS(nbytes) );

    int n;

/*  do { */							// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

    n = read (fd, PTR_CAST(char*, vec), nbytes);

/*  } while (n < 0 && errno == EINTR);	*/			// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    if (n < 0)    	return RAISE_SYSERR(task, n);
    else if (n == 0)	return ZERO_LENGTH_STRING_GLOBAL;

    if (n < nbytes) {
	shrink_fresh_int1_vector( task, vec, BYTES_TO_WORDS(n) );	// Shrink the vector.
    }

    Val                 result;
    SEQHDR_ALLOC (task, result, STRING_TAGWORD, vec, n);
    return              result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

