// fsync.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif



// One of the library bindings exported via
//     src/c/lib/posix-io/cfun-list.h
// and thence
//     src/c/lib/posix-io/libmythryl-posix-io.c



Val   _lib7_P_IO_fsync   (Task* task,  Val arg)   {
    //================
    //
    // Mythryl type:   Sy_Int -> Void
    //
    // Synchronize  a  file's in-core state with storage
    //
    // This fn gets bound as   fsync'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-io.pkg
    //     src/lib/std/src/posix-1003.1b/posix-io-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_IO_fsync");

    int status;
    int fd = TAGGED_INT_TO_C_INT(arg);

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_IO_fsync", arg );
	//
	status = fsync(fd);
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_IO_fsync" );

    RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

