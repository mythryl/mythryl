// fchmod.c


#include "../../config.h"

#include "system-dependent-unix-stuff.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
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
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

    int	   fd   =  GET_TUPLE_SLOT_AS_INT( arg, 0);
    mode_t mode =  TUPLE_GETWORD(arg, 1);
    //
    int status = fchmod (fd, mode);
    //
    CHECK_RETURN_UNIT(task, status)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

