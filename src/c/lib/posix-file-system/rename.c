// rename.c


#include "../../config.h"

#include "system-dependent-unix-stuff.h"
#include <stdio.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_rename   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type: (String, String) -> Void
    //                oldname  newname
    //
    // Change the name of a file
    //
    // This fn gets bound as   rename'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

    Val	oldname = GET_TUPLE_SLOT_AS_VAL(arg, 0);
    Val	newname = GET_TUPLE_SLOT_AS_VAL(arg, 1);

    int status = rename(HEAP_STRING_AS_C_STRING(oldname), HEAP_STRING_AS_C_STRING(newname));

    CHECK_RETURN_UNIT (task, status)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

