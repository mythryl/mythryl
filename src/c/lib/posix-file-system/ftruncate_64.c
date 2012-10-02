// ftruncate_64.c
//
//   Version of ftruncate with 64-positions passed as pair of 32-bit values.


#include "../../mythryl-config.h"

#include <stdio.h>
#include <errno.h>

#include "system-dependent-unix-stuff.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
    #include <sys/stat.h>
#endif



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_ftruncate_64   (Task* task,  Val arg)   {
    //============================
    //
    // Mythryl type: (Int, Unt1,    Unt1) -> Void
    //                fd   lengthhi  lengthlo
    //
    // Make a directory.
    //
    // This fn gets bound as   ftruncate'   in:
    //
    //     src/lib/std/src/psx/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_ftruncate_64");

    int		    fd = GET_TUPLE_SLOT_AS_INT(arg, 0);
    //
    off_t	    len =
      (sizeof(off_t) > 4)
      ? (((off_t)WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg, 1))) << 32) |
        ((off_t)(WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg, 2))))
      : ((off_t)(WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg, 2))));

    int		    status;

/*  do { */										// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

	RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_FileSys_ftruncate_64", NULL);
	    //
	    status = ftruncate (fd, len);						// This call can return EINTR, so it is officially "slow".
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_FileSys_ftruncate_64" );

if (errno == EINTR) puts("Error: EINTR in ftrucate_64.c\n");
/*  } while (status < 0 && errno == EINTR);	*/					// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    return  RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);
}


// Copyright (c) 2004 by The Fellowship of SML/NJ
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

