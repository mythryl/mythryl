// mkfifo.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#include <stdio.h>
#include <string.h>

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
    #include <sys/stat.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"


// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_mkfifo   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type:  (String, Unt) -> Void
    //                 name    mode
    //
    // Make a FIFO special file.
    //
    // This fn gets bound as   make_pipe'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //
    // This fn gets bound as   make_fifo'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_mkfifo");

    int     status;

    Val	    path = GET_TUPLE_SLOT_AS_VAL(  arg, 0);
    mode_t  mode = TUPLE_GETWORD(          arg, 1);
    //
    char*  heap_path = HEAP_STRING_AS_C_STRING( path );
    //
    Mythryl_Heap_Value_Buffer  path_buf;
    //
    {	char* c_path
	    = 
	    buffer_mythryl_heap_value( &path_buf, (void*) heap_path, strlen( heap_path ) +1 );		// '+1' for terminal NUL on string.

	RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_FileSys_mkfifo", NULL );
	    //
	    status = mkfifo (c_path, mode);
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_FileSys_mkfifo" );

	unbuffer_mythryl_heap_value( &path_buf );
    }

    RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

