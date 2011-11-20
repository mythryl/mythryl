// debug.c
//
// Print a string out to the debug stream.


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "cfun-proto-list.h"


// One of the library bindings exported via
//     src/c/lib/heap/cfun-list.h
// and thence
//     src/c/lib/heap/libmythryl-heap.c


Val   _lib7_runtime_debug   (Task* task,  Val arg)   {
    //===================
    //
    // Mythryl type:   String -> Void
    //
    // This fn gets bound to 'debug'     in:   src/lib/std/src/nj/runtime-signals-guts.pkg
    // This fn gets bound to 'say_debug' in:   src/lib/src/lib/thread-kit/src/core-thread-kit/threadkit-debug.pkg
    //     

    char* heap_string = HEAP_STRING_AS_C_STRING(arg);

    Mythryl_Heap_Value_Buffer string_buf;

    char* c_string = buffer_mythryl_heap_value( &string_buf, (void*) heap_string, strlen( heap_string ) +1 );	// '+1' for terminal NUL at end of string.
    
    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_runtime_debug", arg );
	//
        debug_say( c_string );					// debug_say	is from   src/c/main/error-reporting.c
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_runtime_debug" );

    unbuffer_mythryl_heap_value( &string_buf );

    return HEAP_VOID;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


