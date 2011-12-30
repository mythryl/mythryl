// openf.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"


#if HAVE_FCNTL_H
    #include <fcntl.h>
#endif

#include "make-strings-and-vectors-etc.h"
    #include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_openf   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type:  (String, Unt,  Unt) -> int
    //                 name    flags mode
    //
    // Open a file and return the file descriptor.
    //
    // This fn gets bound as   openf'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_openf");

    Val path  = GET_TUPLE_SLOT_AS_VAL(    arg, 0);
    int flags = TUPLE_GETWORD(arg, 1);
    int mode  = TUPLE_GETWORD(arg, 2);

    int	 fd;

    char* heap_path = HEAP_STRING_AS_C_STRING( path );

    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_path into C storage: 
    //
    Mythryl_Heap_Value_Buffer  path_buf;
    //
    {	char* c_path
	    = 
	    buffer_mythryl_heap_value( &path_buf, (void*) heap_path, strlen( heap_path ) +1 );		// '+1' for terminal NUL on string.

    /*  do { */									// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

	    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_openf", arg );
		//
		fd    = open( c_path, flags, mode );
		//
	    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_openf" );

    /*  } while (fd < 0 && errno == EINTR);	*/					// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

	unbuffer_mythryl_heap_value( &path_buf );
    }

    CHECK_RETURN( task, fd )
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

