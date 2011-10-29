// pause.c


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif



// One of the library bindings exported via
//     src/c/lib/posix-process/cfun-list.h
// and thence
//     src/c/lib/posix-process/libmythryl-posix-process.c



Val   _lib7_P_Process_pause   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type: Void -> Void
    //
    // Wait for a POSIX interprocess signal.
    //
    // This fn gets bound as   pause   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-process.pkg

    pause ();								// Documentation in	man 2 pause

    return HEAP_VOID;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

