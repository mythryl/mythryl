// unlink.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#include <stdio.h>
#include <string.h>

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_unlink   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type:   String -> Void
    //
    // Remove directory entry
    //
    // This fn gets bound as   unlink   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_unlink");

    int status;

    char* heap_path = HEAP_STRING_AS_C_STRING(arg);

    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_path into C storage: 
    //
    Mythryl_Heap_Value_Buffer  path_buf;
    //
    {	char* c_path
	    = 
	    buffer_mythryl_heap_value( &path_buf, (void*) heap_path, strlen( heap_path ) +1 );		// '+1' for terminal NUL on string.

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_unlink", arg );
	    //
	    status = unlink( c_path );
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_unlink" );

	unbuffer_mythryl_heap_value( &path_buf );
    }

    CHECK_RETURN_UNIT(task, status)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

