// umask.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#include <stdio.h>
#include <string.h>

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
    #include <sys/stat.h>
#endif

#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_umask   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type : Unt -> Unt
    //
    // Set and get file creation mask
    // Assumes umask never fails.
    //
    // This fn gets bound as   umask'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_umask");

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_umask", NULL );
	//
	mode_t omask = umask(WORD_LIB7toC(arg));
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_umask" );

    return  make_one_word_unt(task, (Vunt) omask  );
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

