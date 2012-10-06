// waitpid.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#if HAVE_SYS_WAIT_H
    #include <sys/wait.h>
#endif

#include "system-dependent-unix-stuff.h"


#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-process/cfun-list.h
// and thence
//     src/c/lib/posix-process/libmythryl-posix-process.c


//       WAIT(2)                    Linux Programmer's Manual                   WAIT(2)
//       
//       NAME
//              wait, waitpid, waitid - wait for process to change state
//       
//       SYNOPSIS
//              #include <sys/types.h>
//              #include <sys/wait.h>
//       
//              pid_t waitpid(pid_t pid, int *status, int options);
//       



Val   _lib7_P_Process_waitpid   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:  (Int, Unt) -> (Int, Int, Int)
    //
    // Wait for child processes to stop or terminate.
    //
    // This fn gets bound as   waitpid'   in:
    //
    //     src/lib/std/src/psx/posix-process.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int status;
    int how;
    int val;

    int  result;

    int pid     = GET_TUPLE_SLOT_AS_INT( arg, 0 );
    int options = TUPLE_GETWORD(         arg, 1 );    

  do { 						// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c
						// Restored 2012-08-01 CrT due to consistent failures here when switching on thread-scheduler-control-g.pkg by default.
        RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_Process_waitpid", NULL );
	    //
	    result = waitpid( pid, &status, options );
	    //
        RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_Process_waitpid" );

  } while (result < 0 && errno == EINTR);		// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    if (result < 0)   return RAISE_SYSERR__MAY_HEAPCLEAN(task, result, NULL);

    if (WIFEXITED(status)) {
	//
	how = 0;
	val = WEXITSTATUS(status);

    } else if (WIFSIGNALED(status)) {

	how = 1;
	val = WTERMSIG(status);

    } else if (WIFSTOPPED(status)) {

	how = 2;
	val = WSTOPSIG(status);

    } else {

        return RAISE_ERROR__MAY_HEAPCLEAN(task, "unknown child status", NULL);
    }

    return   make_three_slot_record(task,  TAGGED_INT_FROM_C_INT(result), TAGGED_INT_FROM_C_INT(how), TAGGED_INT_FROM_C_INT(val) );
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

