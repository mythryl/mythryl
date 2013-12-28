// sysconf.c



#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#include <stdio.h>
#include <string.h>

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

#include <errno.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"
#include "../posix-error/posix-name-val.h"

// The following table is generated from all _SC_ values
// in unistd.h. For most systems, this will include
// _SC_ARG_MAX
// _SC_CHILD_MAX
// _SC_CLK_TCK
// _SC_JOB_CONTROL
// _SC_NGROUPS_MAX
// _SC_OPEN_MAX
// _SC_SAVED_IDS
// _SC_STREAM_MAX
// _SC_TZNAME_MAX
// _SC_VERSION
//
// The full POSIX list is given in section 4.8.1 of Std 1003.1b-1993.
//
// The Mythryl string used to look up these values has the same
// form but without the prefix, e.g., to lookup _SC_ARG_MAX,
// use sysconf "ARG_MAX"
//
static name_val_t   values[]   = {
    //
    #include "ml_sysconf.h"
};

#define NUMELMS ((sizeof values)/(sizeof (name_val_t)))



// One of the library bindings exported via
//     src/c/lib/posix-process-environment/cfun-list.h
// and thence
//     src/c/lib/posix-process-environment/libmythryl-posix-process-environment.c



Val   _lib7_P_ProcEnv_sysconf   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:   String -> Unt
    //
    // Get configurable system variables
    //
    // This fn gets bound as   sysconf   in:
    //
    //     src/lib/std/src/psx/posix-process.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    name_val_t* attribute =  _lib7_posix_nv_binary_search(HEAP_STRING_AS_C_STRING(arg), values, NUMELMS);
    //
    if (!attribute) {
        //
        errno = EINVAL;
        return RAISE_SYSERR__MAY_HEAPCLEAN(task, -1, NULL);
    }
 
    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	long val;
	errno = 0;
	//
	while (((val = sysconf(attribute->val)) == -1) && (errno == EINTR)) {
	    errno = 0;
	    continue;
	}
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    Val result;

    if (val >= 0) {
	//
        result =  make_one_word_unt(task, val );
    } else {

	if (errno == 0)   result = RAISE_ERROR__MAY_HEAPCLEAN(task, "unsupported POSIX feature", NULL);
	else              result = RAISE_SYSERR__MAY_HEAPCLEAN(task, -1, NULL);
    }
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

