// rename.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"
#include <stdio.h>

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



Val    _lib7_P_FileSys_rename   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type: (String, String) -> Void
    //                oldname  newname
    //
    // Change the name of a file
    //
    // This fn gets bound as   rename'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_rename");

    int status;

    Val	oldname = GET_TUPLE_SLOT_AS_VAL(arg, 0);
    Val	newname = GET_TUPLE_SLOT_AS_VAL(arg, 1);

    char* heap_oldname = HEAP_STRING_AS_C_STRING(oldname);
    char* heap_newname = HEAP_STRING_AS_C_STRING(newname);

    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_path into C storage: 
    //
    Mythryl_Heap_Value_Buffer  oldname_buf;
    Mythryl_Heap_Value_Buffer  newname_buf;
    //
    {	char* c_oldname	= buffer_mythryl_heap_value( &oldname_buf, (void*) heap_oldname, strlen( heap_oldname ) +1 );		// '+1' for terminal NUL on string.
	char* c_newname	= buffer_mythryl_heap_value( &newname_buf, (void*) heap_newname, strlen( heap_newname ) +1 );		// '+1' for terminal NUL on string.

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_rename", &arg );
	    //
	    status = rename(c_oldname, c_newname);
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_rename" );

	unbuffer_mythryl_heap_value( &oldname_buf );
	unbuffer_mythryl_heap_value( &newname_buf );
    }

    RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

