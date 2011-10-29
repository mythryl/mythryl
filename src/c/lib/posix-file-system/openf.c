// openf.c


#include "../../mythryl-config.h"

#include <errno.h>

#include "system-dependent-unix-stuff.h"

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

    Val path  = GET_TUPLE_SLOT_AS_VAL(    arg, 0);
    int flags = TUPLE_GETWORD(arg, 1);
    int mode  = TUPLE_GETWORD(arg, 2);

    int		    fd;

/*  do { */					// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

        fd    = open (HEAP_STRING_AS_C_STRING(path), flags, mode);

/*  } while (fd < 0 && errno == EINTR);	*/	// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    CHECK_RETURN(task, fd)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

