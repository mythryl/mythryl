// getgrgid.c


#include "../../config.h"

#include "system-dependent-unix-stuff.h"
#include <stdio.h>
#include <grp.h>
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



Val   _lib7_P_SysDB_getgrgid   (Task* task,  Val arg)   {
    //======================
    //
    // Mythryl type:   Unt -> (String, Unt, List(String))
    //
    // Get group file entry by gid.
    //
    // This fn gets bound as   getgrgid'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-etc.pkg


    struct group* info =  getgrgid( WORD_LIB7toC( arg ));

    if (info == NULL)   return RAISE_SYSERR(task, -1);
  
    Val gr_name =  make_ascii_string_from_c_string (task, info->gr_name);

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

