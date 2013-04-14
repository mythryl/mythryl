// kill.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"
#include <signal.h>

/*
###        "From too much love of living,
###         From hope and fear set free,
###         We thank with brief thanksgiving,
###         Whatever gods may be,
###         That no life lives forever,
###         That dead men rise up never,
###         That even the weariest river
###         Winds somewhere safe to sea."
###
###                     -- Swinburne
*/



// One of the library bindings exported via
//     src/c/lib/posix-process/cfun-list.h
// and thence
//     src/c/lib/posix-process/libmythryl-posix-process.c



//       KILL(2)                    Linux Programmer's Manual                   KILL(2)
//       
//       NAME
//              kill - send signal to a process
//       
//       SYNOPSIS
//              #include <sys/types.h>
//              #include <signal.h>
//       
//              int kill(pid_t pid, int sig);




Val   _lib7_P_Process_kill   (Task* task,  Val arg)   {
    //====================
    //
    // Mythryl type:  (Int, Int) -> Void
    //
    // Send a signal to a process or a group of processes
    //
    // This fn gets bound as   exec   in:
    //
    //     src/lib/std/src/psx/posix-process.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int pid                =  GET_TUPLE_SLOT_AS_INT( arg, 0 );
    int portable_signal_id =  GET_TUPLE_SLOT_AS_INT( arg, 1 );
    int host_os_signal_id  =  portable_signal_id_to_host_os_signal_id( portable_signal_id );				// portable_signal_id_to_host_os_signal_id	is from   src/c/machine-dependent/interprocess-signals.c

    if (!host_os_signal_id) {
	char buf[ 132 ];
	sprintf(buf, "Signal %d not supported on this OS", portable_signal_id);
	return RAISE_ERROR__MAY_HEAPCLEAN(task,buf,NULL);
    }

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	int status = kill( pid, host_os_signal_id );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    Val result =  RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);		// RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN	is from   src/c/lib/raise-error.h

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

