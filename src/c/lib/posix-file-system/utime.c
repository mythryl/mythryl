// utime.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#include <stdio.h>
#include <string.h>

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#if HAVE_UTIME_H
    #include <utime.h>
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



Val   _lib7_P_FileSys_utime   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type: (String, one_word_int::Int, one_word_int::Int) -> Void
    //                name    actime      modtime
    //
    // Sets file access and modification times.
    // If actime = -1, then set both to current time.
    //
    // This fn gets bound as   utime'   in:
    //
    //     src/lib/std/src/psx/posix-file.pkg
    //     src/lib/std/src/psx/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_utime");

    int status;

    Val	    path    =  GET_TUPLE_SLOT_AS_VAL( arg, 0);
    time_t  actime  =  TUPLE_GET_INT1(        arg, 1);
    time_t  modtime =  TUPLE_GET_INT1(        arg, 2);

    char* heap_path =  HEAP_STRING_AS_C_STRING( path );

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


	if (actime == -1) {

	    RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_FileSys_utime", NULL );
		//
		status = utime( c_path, NULL );
		//
	    RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_FileSys_utime" );

	} else {

	    struct utimbuf tb;

	    tb.actime = actime;
	    tb.modtime = modtime;

	    RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_FileSys_utime", NULL );
		//
		status = utime( c_path, &tb );
		//
	    RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_FileSys_utime" );
	}

	unbuffer_mythryl_heap_value( &path_buf );
    }

    return  RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

