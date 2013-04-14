// times.c



#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <sys/times.h>

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-process-environment/cfun-list.h
// and thence
//     src/c/lib/posix-process-environment/libmythryl-posix-process-environment.c



Val   _lib7_P_ProcEnv_times   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type:   Void -> (Int, Int, Int, Int, Int)
    //
    // Return process and child process times, in clock ticks.
    //
    // This fn gets bound as   times'   in:
    //
    //     src/lib/std/src/psx/posix-id.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Val  e;
    Val  u, s;
    Val  cu, cs;

    struct tms   ts;

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	clock_t t = times( &ts );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    if (t == -1)   return RAISE_SYSERR__MAY_HEAPCLEAN(task, -1, NULL);

    e  =  make_one_word_int(task,  t            );
    u  =  make_one_word_int(task,  ts.tms_utime );
    s  =  make_one_word_int(task,  ts.tms_stime );
    cu =  make_one_word_int(task,  ts.tms_cutime);
    cs =  make_one_word_int(task,  ts.tms_cstime);

    Val result = make_five_slot_record(task,  e, u, s, cu, cs  );
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

