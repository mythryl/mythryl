// rmdir.c


#include "../../config.h"

#include "system-dependent-unix-stuff.h"

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-filesys/cfun-list.h
// and thence
//     src/c/lib/posix-filesys/libmythryl-posix-filesys.c



Val   _lib7_P_FileSys_rmdir   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl:   String -> Void
    //
    // Remove a directory
    //
    // This fn gets bound as   rmdir   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-filesys-64.pkg

    int status = rmdir(HEAP_STRING_AS_C_STRING(arg));
    //
    CHECK_RETURN_UNIT(task, status)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

