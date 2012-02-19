// rewinddir.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#include <stdio.h>
#include <string.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_rewinddir   (Task* task,  Val arg)   {
    //=========================
    //
    // Mythryl type:   Chunk -> Void
    //
    // Rewind a directory stream.
    //
    // This fn gets bound as   rewinddir'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_rewinddir");

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_rewinddir", &arg );
	//
	rewinddir(PTR_CAST(DIR*, arg));
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_rewinddir" );

    return HEAP_VOID;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

