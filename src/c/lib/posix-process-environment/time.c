// time.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
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
    //     src/lib/std/src/posix-1003.1b/posix-id.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_ProcEnv_time");

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_time", arg );
	//
	time_t t =  time( NULL );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_ProcEnv_time" );

    return  make_one_word_int(task,  t  );
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

