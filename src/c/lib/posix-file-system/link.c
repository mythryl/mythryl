// link.c


#include "../../mythryl-config.h"

#include <stdio.h>

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
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
    //                  existing newname
    //
    // Creates a hard link from newname to existing file.
    //
    // This fn gets bound as   link'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

    Val	existing =  GET_TUPLE_SLOT_AS_VAL(arg, 0);
    Val	newname  =  GET_TUPLE_SLOT_AS_VAL(arg, 1);

    char* heap_existing =  HEAP_STRING_AS_C_STRING( existing );
    char* heap_newname  =  HEAP_STRING_AS_C_STRING( newname  );

    // We cannot reference anything on the Mythryl
    // heap after we do CEASE_USING_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_path into C storage: 
    //
    Mythryl_Heap_Value_Buffer  existing_buf;    char* c_existing =  buffer_mythryl_heap_value( &existing_buf, (void*) heap_existing, strlen( heap_existing ) +1 );		// '+1' for terminal NUL on string.
    Mythryl_Heap_Value_Buffer   newname_buf;    char* c_newname	 =  buffer_mythryl_heap_value(  &newname_buf, (void*) heap_newname,  strlen( heap_newname  ) +1 );		// '+1' for terminal NUL on string.

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_link", arg );
	//
        int status = link( c_existing, c_newname );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_link" );

    unbuffer_mythryl_heap_value( &existing_buf );
    unbuffer_mythryl_heap_value(  &newname_buf );

    CHECK_RETURN_UNIT (task, status)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

