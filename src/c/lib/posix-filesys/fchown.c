// fchown.c


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
//     src/c/lib/posix-filesys/cfun-list.h
// and thence
//     src/c/lib/posix-filesys/libmythryl-posix-filesys.c



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
    //     src/lib/std/src/posix-1003.1b/posix-filesys-64.pkg

    int	    fd =  GET_TUPLE_SLOT_AS_INT (arg, 0);
    uid_t  uid =  TUPLE_GETWORD(arg, 1);
    gid_t  gid =  TUPLE_GETWORD(arg, 2);
    //
    int status = fchown (fd, uid, gid);
    //
    CHECK_RETURN_UNIT(task, status)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

