// chown.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_chown   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type: (String, Unt, Unt) -> Void
    //                name    uid  gid
    //
    // Change owner and group of file given its name.
    //
    // This fn gets bound as   chown'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_chown");

    int   status;

    Val	  path = GET_TUPLE_SLOT_AS_VAL(    arg, 0);
    uid_t uid  = TUPLE_GETWORD(arg, 1);
    gid_t gid  = TUPLE_GETWORD(arg, 2);
    char* heap_path=  HEAP_STRING_AS_C_STRING(path);


    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_path into C storage: 
    //
    Mythryl_Heap_Value_Buffer  path_buf;
    //
    {	char* c_path
	    = 
	    buffer_mythryl_heap_value( &path_buf, (void*) path, strlen( heap_path ) +1 );		// '+1' for terminal NUL on string.

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_chown", &arg );
	    //
	    status = chown (c_path, uid, gid);
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_chown" );
	//
	unbuffer_mythryl_heap_value( &path_buf );
    }

    RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

