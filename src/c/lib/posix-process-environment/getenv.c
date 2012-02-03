// getenv.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"



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

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_ProcEnv_getenv");

    char* status;

    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_path into C storage: 
    //
    Mythryl_Heap_Value_Buffer  key_buf;
    {
	char* heap_key = HEAP_STRING_AS_C_STRING( arg );

	char* c_key
	    = 
	    buffer_mythryl_heap_value( &key_buf, (void*) heap_key, strlen( heap_key ) +1 );		// '+1' for terminal NUL on string.

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_getenv", arg );
	    //
	    status = getenv( c_key );
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_getenv" );

	unbuffer_mythryl_heap_value( &key_buf );
    }

    if (status == NULL)   return OPTION_NULL;

    Val s = make_ascii_string_from_c_string( task, status);			// make_ascii_string_from_c_string	def in    src/c/heapcleaner/make-strings-and-vectors-etc.c

    return  OPTION_THE( task, s );
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

