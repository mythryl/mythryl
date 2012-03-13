// fchown.c


#include "../../mythryl-config.h"

#include <stdio.h>

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_fchown   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type: (Int, Unt, Unt) -> Void
    //                fd   uid  gid
    //
    // Change owner and group of file given a file descriptor for it.
    //
    // This fn gets bound as   fchown'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_fchown");

    int	    fd =  GET_TUPLE_SLOT_AS_INT( arg, 0);
    uid_t  uid =  TUPLE_GETWORD(         arg, 1);
    gid_t  gid =  TUPLE_GETWORD(         arg, 2);

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_fchown", NULL );
	//
        int status = fchown (fd, uid, gid);
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_fchown" );

    RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

