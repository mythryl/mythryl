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
#include "lib7-c.h"
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
    //     src/lib/std/src/posix-1003.1b/posix-process.pkg

    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_Process_kill");

    int pid =  GET_TUPLE_SLOT_AS_INT(arg, 0);
    int sig =  GET_TUPLE_SLOT_AS_INT(arg, 1);

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_Process_kill", arg );
	//
	int status = kill( pid, sig );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_Process_kill" );

    CHECK_RETURN_UNIT (task, status)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

