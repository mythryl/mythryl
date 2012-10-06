// geterror.c
//
// Return the system constant that corresponds to the given error name.


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

extern Sysconsts	errno_table__global;



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
    //     src/lib/std/src/psx/posix-error.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Val result =  make_system_constant__may_heapclean( task, &errno_table__global, TAGGED_INT_TO_C_INT(arg), NULL );		// make_system_constant__may_heapclean		def in    src/c/heapcleaner/make-strings-and-vectors-etc.c

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

