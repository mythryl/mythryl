// exece.c



#include "../../config.h"

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



Val   _lib7_P_Process_exece   (Task* task,  Val arg)   {
    //=====================
    //
    // _lib7_P_Process_exece : String * String list * String list -> 'a
    //
    // Overlay a new process image, using specified environment.
    //
    // This fn gets bound as   exece   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-process.pkg

    Val  path   =  GET_TUPLE_SLOT_AS_VAL( arg, 0 );
    Val  arglst =  GET_TUPLE_SLOT_AS_VAL( arg, 1 );
    Val	 envlst =  GET_TUPLE_SLOT_AS_VAL( arg, 2 );

    // Use the heap for temp space for
    // the argv[] and envp[] vectors:
    //
    char** cp =  (char**)(task->heap_allocation_pointer);

    #ifdef SIZES_C_64_MYTHRYL_32
	//
	// 8-byte align it:
	//
	cp = (char**) ROUND_UP_TO_POWER_OF_TWO((Unt64)cp, POINTER_BYTESIZE);
    #endif

    char** argv = cp;
    //
    for (Val p = arglst;  p != LIST_NIL;  p = LIST_TAIL(p)) {
        *cp++ = HEAP_STRING_AS_C_STRING(LIST_HEAD(p));
    }
    *cp++ = 0;						// Terminate the argv[].

    char** envp = cp;
    //
    for (Val p = envlst;  p != LIST_NIL;  p = LIST_TAIL(p)) {
        *cp++ = HEAP_STRING_AS_C_STRING(LIST_HEAD(p));
    }
    *cp++ = 0;						// Terminate the envp[].

    int status =  execve( HEAP_STRING_AS_C_STRING(path), argv, envp );

    CHECK_RETURN (task, status)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

