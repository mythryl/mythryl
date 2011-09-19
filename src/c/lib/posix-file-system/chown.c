// chown.c


#include "../../config.h"

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
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



Val   _lib7_P_FileSys_chown   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type: (String, Unt, Unt) -> Void
    //                name    uid  gid
    //
    // Change owner and group of file given its name.
    //
    // This fn gets bound as   chown'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

    Val	  path = GET_TUPLE_SLOT_AS_VAL(    arg, 0);
    uid_t uid  = TUPLE_GETWORD(arg, 1);
    gid_t gid  = TUPLE_GETWORD(arg, 2);
    //
    int status = chown (HEAP_STRING_AS_C_STRING(path), uid, gid);
    //
    CHECK_RETURN_UNIT(task, status)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

