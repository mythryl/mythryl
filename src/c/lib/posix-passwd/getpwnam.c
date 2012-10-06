// getpwnam.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <pwd.h>

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"
#include <pwd.h>



// One of the library bindings exported via
//     src/c/lib/posix-passwd/cfun-list.h
// and thence
//     src/c/lib/posix-passwd/libmythryl-posix-passwd-db.c



Val   _lib7_P_SysDB_getpwnam   (Task* task,  Val arg)   {
    //======================
    //
    // _lib7_P_SysDB_getpwnam : String -> (String, word, word, String, String)
    //
    // Get password file entry by name.
    //
    // This fn gets bound as   getpwnam'   in:
    //
    //     src/lib/std/src/psx/posix-etc.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);


    struct passwd*  info;

    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_path into C storage: 
    //
    Mythryl_Heap_Value_Buffer  name_buf;
    //
    {	char* heap_name = HEAP_STRING_AS_C_STRING( arg );

	char* c_name
	    = 
	    buffer_mythryl_heap_value( &name_buf, (void*) heap_name, strlen( heap_name ) +1 );		// '+1' for terminal NUL on string.

	RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_SysDB_getpwnam", NULL );
	    //
	    info =  getpwnam( c_name );
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_SysDB_getpwnam" );

	unbuffer_mythryl_heap_value( &name_buf );
    }

    if (info == NULL)   return RAISE_SYSERR__MAY_HEAPCLEAN(task, -1, NULL);
  
    Val pw_name  =  make_ascii_string_from_c_string__may_heapclean(	task,                  info->pw_name, NULL	);		Roots roots1 = { &pw_name, NULL };
    Val pw_uid   =  make_one_word_unt(					task, (Vunt) (info->pw_uid)		);		Roots roots2 = { &pw_uid,  &roots1 };
    Val pw_gid   =  make_one_word_unt(					task, (Vunt) (info->pw_gid)		);		Roots roots3 = { &pw_gid,  &roots2 };
    Val pw_dir   =  make_ascii_string_from_c_string__may_heapclean(	task,                  info->pw_dir,   &roots3	);		Roots roots4 = { &pw_dir,  &roots3 };
    Val pw_shell =  make_ascii_string_from_c_string__may_heapclean(	task,                  info->pw_shell, &roots4	);

    Val result   =  make_five_slot_record(task,  pw_name, pw_uid, pw_gid, pw_dir, pw_shell  );

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

