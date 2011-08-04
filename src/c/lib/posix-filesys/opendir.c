// opendir.c


#include "../../config.h"

#include "system-dependent-unix-stuff.h"

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
#include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-filesys/cfun-list.h
// and thence
//     src/c/lib/posix-filesys/libmythryl-posix-filesys.c



Val   _lib7_P_FileSys_opendir   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:  String -> Chunk
    //
    // Open and return a directory stream.
    //
    // This fn gets bound as   opendir'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-filesys-64.pkg

    DIR* dir = opendir(HEAP_STRING_AS_C_STRING(arg));
    //
    if (dir == NULL)  return RAISE_SYSERR(task, -1);

    return PTR_CAST( Val, dir);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

