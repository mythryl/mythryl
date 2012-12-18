// closedir.c


#include "../../mythryl-config.h"

#include <stdio.h>

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#endif



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_closedir   (Task* task,  Val arg) {
    //========================
    //
    // Mythryl type:  Chunk -> Void
    //
    // Close a directory stream.
    //
    // This fn gets bound as   closedir'   in:
    //
    //     src/lib/std/src/psx/posix-file.pkg
    //     src/lib/std/src/psx/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, &arg );
	//
        int status = closedir(PTR_CAST(DIR*, arg));				// Note that closedir() arg here does not actually reference Mythryl heap (forbidden) -- check code in src/c/lib/posix-file-system/opendir.c
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );
    //
    Val result = RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);

    									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

