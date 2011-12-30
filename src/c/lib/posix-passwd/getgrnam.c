// getgrnam.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <grp.h>

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-passwd/cfun-list.h
// and thence
//     src/c/lib/posix-passwd/libmythryl-posix-passwd-db.c



Val   _lib7_P_SysDB_getgrnam   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type:   String -> (String, Unt, List(String))
    //
    // Get group file entry by name.
    //
    // This fn gets bound as   getgrname'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-etc.pkg

    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_SysDB_getgrnam");

    struct group*  info;


    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_name into C storage: 
    //
    Mythryl_Heap_Value_Buffer  name_buf;
    //
    {   char* heap_name =  HEAP_STRING_AS_C_STRING( arg );

   	char* c_name
	    = 
	    buffer_mythryl_heap_value( &name_buf, (void*) heap_name, strlen( heap_name ) +1 );		// '+1' for terminal NUL on string.


	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_SysDB_getgrnam", arg );
	    //
	    info =  getgrnam( c_name );
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_SysDB_getgrnam" );

	unbuffer_mythryl_heap_value( &name_buf );
    }
    if (info == NULL)   return RAISE_SYSERR(task, -1);
  
    Val  gr_name =  make_ascii_string_from_c_string( task, info->gr_name );

    Val               gr_gid;
    WORD_ALLOC (task, gr_gid, (Val_Sized_Unt)(info->gr_gid));

    Val gr_mem =  make_ascii_strings_from_vector_of_c_strings( task, info->gr_mem );

    Val              result;
    REC_ALLOC3(task, result, gr_name, gr_gid, gr_mem);
    return           result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

