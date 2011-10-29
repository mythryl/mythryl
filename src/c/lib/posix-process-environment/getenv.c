// getenv.c


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"
#include <stdio.h>



// One of the library bindings exported via
//     src/c/lib/posix-process-environment/cfun-list.h
// and thence
//     src/c/lib/posix-process-environment/libmythryl-posix-process-environment.c



Val   _lib7_P_ProcEnv_getenv   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type:  String -> Null_Or(String)
    //
    // Return value for environment name
    //
    // This fn gets bound as   getenv   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-id.pkg


    char* status = getenv( HEAP_STRING_AS_C_STRING(arg) );

    if (status == NULL)   return OPTION_NULL;

    Val s = make_ascii_string_from_c_string( task, status);			// make_ascii_string_from_c_string	def in    src/c/heapcleaner/make-strings-and-vectors-etc.c

    Val               result;
    OPTION_THE(task, result, s);
    return            result;
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

