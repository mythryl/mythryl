// ftruncate.c


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



Val   _lib7_P_FileSys_ftruncate   (Task* task,  Val arg)   {
    //=========================
    //
    // Mythryl type: (Int, Int) -> Void
    //                fd   length
    //
    // This fn gets bound as   ftruncate'   in:
    //
    //     src/lib/std/src/psx/posix-file.pkg
											ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
    int	    fd = GET_TUPLE_SLOT_AS_INT(arg, 0);
    off_t  len = GET_TUPLE_SLOT_AS_INT(arg, 1);

    int	    status;

    do {
        RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	    //
	    status = ftruncate (fd, len);						// This call can return EINTR, so it is officially "slow".
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );
	//
    } while (status < 0 && errno == EINTR);						// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    Val result = RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);

											EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.

