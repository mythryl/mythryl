// access.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#include <stdio.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "runtime-base.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"


// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_access   (Task* task,  Val arg) {
    //======================
    //
    // Mythryl type:  (String, Unt) -> Bool
    //                 name    Access_Mode
    //
    // Determine accessibility of a file.
    //
    // This fn gets bound as   access'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_access");

    int    status;

    Val	   path =  GET_TUPLE_SLOT_AS_VAL( arg, 0 );
    mode_t mode =  TUPLE_GETWORD(         arg, 1 );

    char* heap_path = HEAP_STRING_AS_C_STRING( path );

    Mythryl_Heap_Value_Buffer  path_buf;

    {	char* c_path = buffer_mythryl_heap_value( &path_buf, (void*) heap_path, strlen( heap_path ) +1 );		// '+1' for terminal NUL on string.

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_access", arg );
	    //
	    status =  access( c_path, mode );
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_access" );

	unbuffer_mythryl_heap_value( &path_buf );
    }

    if (status == 0)    return HEAP_TRUE;
    else		return HEAP_FALSE;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

