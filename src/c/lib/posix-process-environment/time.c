// time.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-process-environment/cfun-list.h
// and thence
//     src/c/lib/posix-process-environment/libmythryl-posix-process-environment.c



Val   _lib7_P_ProcEnv_time   (Task* task,  Val arg)   {
    //====================
    //
    // Mythryl type:  Void -> one_word_int::Int
    //
    // Return time in seconds from 00:00:00 UTC, January 1, 1970
    //
    // This fn gets bound as   time   in:
    //
    //     src/lib/std/src/psx/posix-id.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	time_t t =  time( NULL );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    Val result =  make_one_word_int(task,  t  );

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

