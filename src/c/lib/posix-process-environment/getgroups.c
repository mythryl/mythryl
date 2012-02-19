// getgroups.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>

#if HAVE_LIMITS_H
    #include <limits.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

// some OSs use int[] as the second argument to getgroups(),
// when gid_t is not int.
//
#ifdef INT_GIDLIST
    typedef int  gid;
#else
    typedef gid_t gid;
#endif


static Val   mkList   (Task* task,  int ngrps,  gid gidset[])   {
    //       ======
    //
    // Convert array of gid_t into a list of gid_t

 
    Val  w;

    // NOTE: We should do something about possible cleaning!!! XXX BUGGO FIXME

    Val p = LIST_NIL;

    while (ngrps --> 0) {
	//
        w = make_one_word_unt(task,  (Val_Sized_Unt)(gidset[ngrps])  );
	//
	p = LIST_CONS(task, w, p);
    }

    return p;
}



// One of the library bindings exported via
//     src/c/lib/posix-process-environment/cfun-list.h
// and thence
//     src/c/lib/posix-process-environment/libmythryl-posix-process-environment.c



Val   _lib7_P_ProcEnv_getgroups   (Task* task,  Val arg)   {
    //=========================
    //
    // Mythryl type:  Void -> List(Int)
    //
    // Return supplementary group access list ids.
    //
    // This fn gets bound as   get_group_ids   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-id.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_ProcEnv_getgroups");

    gid gidset[ NGROUPS_MAX ];

    Val	result;

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_getgroups", arg );
	//
	int ngrps =  getgroups( NGROUPS_MAX, gidset );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_getgroups" );

    if (ngrps != -1) {
	//
	result = mkList (task, ngrps, gidset);

    } else {

	gid* gp;

	// If the error was not due to too small buffer size,
	// raise exception.
	//
	if (errno != EINVAL)   return RAISE_SYSERR__MAY_HEAPCLEAN(task, -1, NULL);

        // Find out how many groups there
        // are and allocate enough space:
        //
	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_getgroups", arg );
	    //
	    ngrps = getgroups( 0, gidset );
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_getgroups" );
	//
	gp = (gid*) MALLOC( ngrps * (sizeof (gid)) );
	//
	if (gp == 0) {
	    errno = ENOMEM;
	    return RAISE_SYSERR__MAY_HEAPCLEAN(task, -1, NULL);
	}

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_getgroups", arg );
	    //
	    ngrps = getgroups (ngrps, gp);
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_getgroups" );

	if (ngrps == -1)   result = RAISE_SYSERR__MAY_HEAPCLEAN(task, -1, NULL);
	else		   result = mkList (task, ngrps, gp);
        
	FREE ((void *)gp);
    }

    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

