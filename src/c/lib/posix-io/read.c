// read.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "system-dependent-unix-stuff.h"

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
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

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_IO_read");

    Val vec;
    int n;

    int fd     =  GET_TUPLE_SLOT_AS_INT(arg, 0);
    int nbytes =  GET_TUPLE_SLOT_AS_INT(arg, 1);

    if (nbytes == 0)   return ZERO_LENGTH_STRING__GLOBAL;

    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so allocate C space in which to do the read:
    //
    Mythryl_Heap_Value_Buffer  vec_buf;
    //
    {	char* c_vec
	    = 
	    buffer_mythryl_heap_nonvalue( &vec_buf, nbytes );


    /*  do { */							// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_IO_read", &arg );
	    //
	    n = read (fd, c_vec, nbytes);
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_IO_read" );

    /*  } while (n < 0 && errno == EINTR);	*/			// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

	if (n <  0)    	{ unbuffer_mythryl_heap_value( &vec_buf );	return RAISE_SYSERR__MAY_HEAPCLEAN(task, n, NULL);		}
	if (n == 0)	{ unbuffer_mythryl_heap_value( &vec_buf );	return ZERO_LENGTH_STRING__GLOBAL;				}

	// Allocate the vector.
	// Note that this might trigger a heapcleaning, moving things around:
	//
	vec = allocate_nonempty_wordslots_vector__may_heapclean( task, BYTES_TO_WORDS(n), NULL );

	memcpy( PTR_CAST(char*, vec), c_vec, n );
    
//	if (n < nbytes) {						// Left-over hack from old code that created vector before doing the read.
//	    shrink_fresh_wordslots_vector( task, vec, BYTES_TO_WORDS(n) );	// Shrink the vector.
//	}

	unbuffer_mythryl_heap_value( &vec_buf );
    }

    return  make_vector_header(task,  STRING_TAGWORD, vec, n);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

