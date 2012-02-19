// getpwuid.c


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
#include "lib7-c.h"
#include "cfun-proto-list.h"


// One of the library bindings exported via
//     src/c/lib/posix-passwd/cfun-list.h
// and thence
//     src/c/lib/posix-passwd/libmythryl-posix-passwd-db.c



Val   _lib7_P_SysDB_getpwuid   (Task* task,  Val arg)   {
    //======================
    //
    // Mytyryl type:   Unt -> (String, Unt, Unt, String, String)
    //
    // Get password file entry by uid.
    //
    // This fn gets bound as   getpwuid'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-etc.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_SysDB_getpwuid");

    struct passwd*  info;

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_SysDB_getpwuid", arg );
	//
	info =  getpwuid( WORD_LIB7toC( arg ));
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_SysDB_getpwuid" );

    if (info == NULL)   return RAISE_SYSERR__MAY_HEAPCLEAN(task, -1, NULL);
  
    Val pw_name  =  make_ascii_string_from_c_string__may_heapclean(	task,                  info->pw_name,  NULL		);		Roots extra_roots1 = { &pw_name, NULL };
    Val pw_uid   =  make_one_word_unt(					task,  (Val_Sized_Unt) (info->pw_uid)			);		Roots extra_roots2 = { &pw_uid,  &extra_roots1 };
    Val pw_gid   =  make_one_word_unt(					task,  (Val_Sized_Unt) (info->pw_gid)			);		Roots extra_roots3 = { &pw_gid,  &extra_roots2 };
    Val pw_dir   =  make_ascii_string_from_c_string__may_heapclean(	task,                   info->pw_dir,   &extra_roots3	);		Roots extra_roots4 = { &pw_dir,  &extra_roots3 };
    Val pw_shell =  make_ascii_string_from_c_string__may_heapclean(	task,                   info->pw_shell, &extra_roots4	);

    return make_five_slot_record(task,  pw_name, pw_uid, pw_gid, pw_dir, pw_shell  );
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

