// fchmod.c


#include "../../mythryl-config.h"

#include <stdio.h>

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



Val   _lib7_P_FileSys_fchmod   (Task* task,  Val arg) {
    //======================
    //
    // Mythryl type: (Fd, Unt) -> Void
    //                fd  mode
    //
    // Change mode of file.
    //
    // This fn gets bound as   fchmod'   in:
    //
    //     src/lib/std/src/psx/posix-file.pkg
    //     src/lib/std/src/psx/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int	   fd   =  GET_TUPLE_SLOT_AS_INT( arg, 0);
    mode_t mode =  TUPLE_GETWORD(         arg, 1);

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
        int status = fchmod (fd, mode);
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    Val result = RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

