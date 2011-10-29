// environ.c


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"



// One of the library bindings exported via
//     src/c/lib/posix-process-environment/cfun-list.h
// and thence
//     src/c/lib/posix-process-environment/libmythryl-posix-process-environment.c



Val   _lib7_P_ProcEnv_environ   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type: Void -> List(String)
    //
    // This fn gets bound as   environ   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-id.pkg

    extern char** environ;
    //
    return make_ascii_strings_from_vector_of_c_strings (task, environ);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

