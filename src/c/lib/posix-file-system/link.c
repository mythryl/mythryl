// link.c


#include "../../mythryl-config.h"

#include <stdio.h>

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif


// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_link   (Task* task,  Val arg)   {
    //====================
    //
    // Mythryl type:   (String, String) -> Void
    //                  existing new_name
    //
    // Creates a hard link from new_name to existing file.
    //
    // This fn gets bound as   link'   in:
    //
    //     src/lib/std/src/psx/posix-file.pkg
    //     src/lib/std/src/psx/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int status;

    Val	existing =  GET_TUPLE_SLOT_AS_VAL(arg, 0);
    Val	new_name =  GET_TUPLE_SLOT_AS_VAL(arg, 1);

    char* heap_existing =  HEAP_STRING_AS_C_STRING( existing );
    char* heap_new_name =  HEAP_STRING_AS_C_STRING( new_name );

    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_path into C storage: 
    //
    Mythryl_Heap_Value_Buffer  existing_buf;
    Mythryl_Heap_Value_Buffer  new_name_buf;

    {	char* c_existing =  buffer_mythryl_heap_value( &existing_buf, (void*) heap_existing, strlen( heap_existing ) +1 );		// '+1' for terminal NUL on string.
	char* c_new_name =  buffer_mythryl_heap_value( &new_name_buf, (void*) heap_new_name, strlen( heap_new_name ) +1 );		// '+1' for terminal NUL on string.

	RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	    //
	    status = link( c_existing, c_new_name );
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

	unbuffer_mythryl_heap_value( &existing_buf );
	unbuffer_mythryl_heap_value( &new_name_buf );
    }

    Val result = RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

