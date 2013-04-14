// osval.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#if HAVE_SYS_WAIT_H
    #include <sys/wait.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"
#include "../posix-error/posix-name-val.h"

// static name_val_t values [] = {
//   {"WNOHANG",       WNOHANG},
// #ifdef WUNTRACED
//   {"WUNTRACED",     WUNTRACED},
// #endif
// };

#define NUMELMS ((sizeof values)/(sizeof (name_val_t)))



// One of the library bindings exported via
//     src/c/lib/posix-process/cfun-list.h
// and thence
//     src/c/lib/posix-process/libmythryl-posix-process.c



Val   _lib7_P_Process_osval   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type:   String -> Int
    //
    // Return the OS-dependent, compile-time constant specified by the string.
    //
    // This fn gets bound as   osval   in:
    //
    //     src/lib/std/src/psx/posix-process.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

// Commented out 2013-03-19 CrT because it seems to always return "not defined" anyhow
// and for the moment I just want a fn which returns a trivial value.
//    name_val_t* resultt =  _lib7_posix_nv_binary_search (HEAP_STRING_AS_C_STRING(arg), values, NUMELMS);

//    Val result;
    //
//    if (resultt)   result =  TAGGED_INT_FROM_C_INT(resultt->val);
//    else           result =  RAISE_ERROR__MAY_HEAPCLEAN(task, "system constant not defined", NULL);

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
//    return result;
return TAGGED_INT_FROM_C_INT(0);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

