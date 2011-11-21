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


    struct passwd*  info;

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_SysDB_getpwuid", arg );
	//
	info =  getpwuid( WORD_LIB7toC( arg ));
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_SysDB_getpwuid" );

    if (info == NULL)   return RAISE_SYSERR(task, -1);
  
    Val pw_name = make_ascii_string_from_c_string (task, info->pw_name);

    Val               pw_uid;
    WORD_ALLOC (task, pw_uid, (Val_Sized_Unt)(info->pw_uid));

    Val               pw_gid;
    WORD_ALLOC (task, pw_gid, (Val_Sized_Unt)(info->pw_gid));

    Val pw_dir   =  make_ascii_string_from_c_string( task, info->pw_dir   );
    Val pw_shell =  make_ascii_string_from_c_string( task, info->pw_shell );

    Val              result;
    REC_ALLOC5(task, result, pw_name, pw_uid, pw_gid, pw_dir, pw_shell);
    return           result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

