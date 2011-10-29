// waitpid.c


#include "../../mythryl-config.h"

#include <errno.h>

#include "system-dependent-unix-stuff.h"

#if HAVE_SYS_WAIT_H
    #include <sys/wait.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-process/cfun-list.h
// and thence
//     src/c/lib/posix-process/libmythryl-posix-process.c



Val   _lib7_P_Process_waitpid   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:  (Int, Unt) -> (Int, Int, Int)
    //
    // Wait for child processes to stop or terminate.
    //
    // This fn gets bound as   waitpid'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-process.pkg

    int status;
    int how;
    int val;

    int  pid;

/*  do { */						// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

        pid = waitpid(GET_TUPLE_SLOT_AS_INT(arg, 0), &status, TUPLE_GETWORD(arg, 1));

/*  } while (pid < 0 && errno == EINTR);	*/	// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    if (pid < 0)   return RAISE_SYSERR(task, pid);

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

        return RAISE_ERROR(task, "unknown child status");
    }

    Val              result;
    REC_ALLOC3(task, result, TAGGED_INT_FROM_C_INT(pid), TAGGED_INT_FROM_C_INT(how), TAGGED_INT_FROM_C_INT(val));
    return           result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

