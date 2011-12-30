// geterror.c
//
// Return the system constant that corresponds to the given error name.


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

extern System_Constants_Table	errno_table__global;



// One of the library bindings exported via
//     src/c/lib/posix-error/cfun-list.h
// and thence
//     src/c/lib/posix-error/libmythryl-posix-error.c



Val   _lib7_P_Error_geterror   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type:   Int -> System_Constant
    //
    // This fn get bound as   geterror   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-error.pkg

    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_Error_geterror");

    return   make_system_constant( task, &errno_table__global, TAGGED_INT_TO_C_INT(arg) );		// make_system_constant		def in    src/c/heapcleaner/make-strings-and-vectors-etc.c
}


// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

