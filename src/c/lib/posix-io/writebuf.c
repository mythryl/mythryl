// writebuf.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "system-dependent-unix-stuff.h"

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

#include "runtime-base.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-io/cfun-list.h
// and thence
//     src/c/lib/posix-io/libmythryl-posix-io.c



Val   _lib7_P_IO_writebuf   (Task* task,  Val arg)   {
    //===================
    //
    // Mythryl type:   (Int, rw_vector_of_one_byte_unts::Rw_Vector, Int, Int) -> Int
    //                  fd    data                      nbytes start              
    //
    // Write nbytes of data from the given rw_vector to the specified file, 
    // starting at the given offset. Assume bounds have been checked.
    //
    // This fn gets bound as   writevec', writearr'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-io.pkg
    //     src/lib/std/src/posix-1003.1b/posix-io-64.pkg

    int		fd         = GET_TUPLE_SLOT_AS_INT(                                 arg, 0);
    Val		start      = GET_TUPLE_SLOT_AS_VAL(                                 arg, 1);
    size_t	nbytes     = GET_TUPLE_SLOT_AS_INT(                                 arg, 2);
    char*	heap_data  = HEAP_STRING_AS_C_STRING(start) + GET_TUPLE_SLOT_AS_INT(arg, 3);


    ssize_t    	n;

    // We cannot reference anything on the Mythryl
    // heap after we do RELEASE_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_data into C storage: 
    //
    Mythryl_Heap_Value_Buffer  data_buf;
    //
    {	char* c_data
	    = 
	    buffer_mythryl_heap_value( &data_buf, (void*) heap_data, nbytes );

/*  do { */					// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_IO_writebuf", arg );
	    //
	    n = write (fd, c_data, nbytes);
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_IO_writebuf" );

/*  } while (n < 0 && errno == EINTR);	*/	// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

	unbuffer_mythryl_heap_value( &data_buf );
    }

    CHECK_RETURN (task, n)
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

