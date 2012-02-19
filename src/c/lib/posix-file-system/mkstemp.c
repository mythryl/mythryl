// openf.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "system-dependent-unix-stuff.h"

#if HAVE_FCNTL_H
    #include <fcntl.h>
#endif

#include "make-strings-and-vectors-etc.h"
    #include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_mkstemp   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:  Void -> Int
    //
    // Open a temporary file and return the file descriptor.
    //
    // This fn gets bound as   tmpfile'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_mkstemp");

    char buf[ 32 ];

    int	 fd;

    strcpy( buf, "tmpfile.XXXXXX" );		// Must end with exactly six 'X's -- see man mkstemp(3).

/*  do { */					// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

    RELEASE_MYTHRYL_HEAP( task->pthread, "", arg );
	//
	fd  =  mkstemp( buf );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "" );

/*  } while (fd < 0 && errno == EINTR);	*/	// Restart if interrupted by a SIGALRM or SIGCHLD or whatever. HAVEN"T CHECKED WHETHER mkstemp IS INTERRUPTABLE -- this is copied blindly from openf.c.


    RETURN_VAL_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, fd, TAGGED_INT_FROM_C_INT( fd ), NULL);
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

