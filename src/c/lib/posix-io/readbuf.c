// readbuf.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-io/cfun-list.h
// and thence
//     src/c/lib/posix-io/libmythryl-posix-io.c



Val   _lib7_P_IO_readbuf   (Task* task,  Val arg)   {
    //==================
    //
    // Mythryl type: (Int, rw_vector_of_one_byte_unts::Rw_Vector, Int,   Int) -> Int
    //                fd   data                       nbytes start
    //
    // Read nbytes of data from the specified file
    // into the given array starting at start.
    // Return the number of bytes read.
    // Assume bounds have been checked.
    //
    // This fn gets bound as   readbuf'   in:
    //
    //     src/lib/std/src/psx/posix-io.pkg
    //     src/lib/std/src/psx/posix-io-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int	  fd     =  GET_TUPLE_SLOT_AS_INT( arg, 0 );
//  Val	  buf    =  GET_TUPLE_SLOT_AS_VAL( arg, 1 );	// We'll do this after the read().
    int	  nbytes =  GET_TUPLE_SLOT_AS_INT( arg, 2 );
//  int   offset =  GET_TUPLE_SLOT_AS_INT( arg, 3 );	// We'll do this after the read().

    int  n;

    Mythryl_Heap_Value_Buffer  vec_buf;

    {	char* c_vec								// Get a pointer to 'nbytes' of free ram outside the Mythryl heap
	    = 									// (i.e., ram guaranteed not to move around during a heapcleaning).
	    buffer_mythryl_heap_nonvalue( &vec_buf, nbytes );

    do {									// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c
										// Restored   2010-10-19 CrT
	RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_IO_readbuf", &arg );	// 'arg' is still live here! 
	    //
	    n = read( fd, c_vec, nbytes );
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_IO_readbuf" );

// if (errno == EINTR) puts("Error: EINTR in readbuf.c\n");
    } while (n < 0 && errno == EINTR);						// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

	// The heapcleaner may have moved everything around
	// during our read() call, so we wait until now to
	// track down the location of our buf vector:
	//
	Val   buf    =                                GET_TUPLE_SLOT_AS_VAL( arg, 1 );
	char* start  = HEAP_STRING_AS_C_STRING(buf) + GET_TUPLE_SLOT_AS_INT( arg, 3 );

	// Copy the bytes read into given
	// string 'buf' on Mythryl heap:
	//
	memcpy( start, c_vec, n );						// Caller is responsible for guaranteeing that this will not overrun the vector and clobber the Mythryl heap.

	unbuffer_mythryl_heap_value( &vec_buf );
    }

    Val result =  RETURN_STATUS_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, n, NULL);

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

