// geteuid.c



#include "../../config.h"

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-procenv/cfun-list.h
// and thence
//     src/c/lib/posix-procenv/libmythryl-posix-process-environment.c



Val   _lib7_P_ProcEnv_geteuid   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:  Void -> Unt
    //
    // Return effective user id
    //
    // This fn gets bound as   get_effective_user_id   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-id.pkg

    Val	              result;
    WORD_ALLOC (task, result, (Val_Sized_Unt)(geteuid()));
    return            result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

