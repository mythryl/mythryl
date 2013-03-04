// rmdir.c


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
#include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_rmdir   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl:   String -> Void
    //
    // Remove a directory
    //
    // This fn gets bound as   rmdir   in:
    //
    //     src/lib/std/src/psx/posix-file.pkg
    //     src/lib/std/src/psx/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int status;

    char* heap_dir =  HEAP_STRING_AS_C_STRING( arg );

    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_path into C storage: 
    //
    Mythryl_Heap_Value_Buffer  dir_buf;
    //
    {   char* c_dir
	    = 
	    buffer_mythryl_heap_value( &dir_buf, (void*) heap_dir, strlen( heap_dir ) +1 );		// '+1' for terminal NUL on string.


	RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	    //
	    status = rmdir( c_dir );
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

	unbuffer_mythryl_heap_value( &dir_buf );
    }

    Val result = RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.

