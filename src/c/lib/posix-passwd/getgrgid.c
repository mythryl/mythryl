// getgrgid.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <grp.h>
#include <string.h>

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
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
    //     src/lib/std/src/psx/posix-etc.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	struct group* info =  getgrgid( WORD_LIB7toC( arg ));
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    if (info == NULL)   return RAISE_SYSERR__MAY_HEAPCLEAN(task, -1, NULL);
  
    Val gr_name =  make_ascii_string_from_c_string__may_heapclean(		task,         info->gr_name, NULL   );		Roots roots1 = { &gr_name, NULL };
    Val gr_gid  =  make_one_word_unt(						task,  (Vunt)(info->gr_gid)	    );		Roots roots2 = { &gr_gid,  &roots1 };
    Val gr_mem  =  make_ascii_strings_from_vector_of_c_strings__may_heapclean(	task,         info->gr_mem, &roots2 );

    Val result =  make_three_slot_record(task,  gr_name, gr_gid, gr_mem  );

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.

