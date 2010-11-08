/* waitpid.c
 *
 */

#include "../../config.h"

#include <errno.h>

#include "runtime-unixdep.h"

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_Process_waitpid : (Int, Unt) -> (Int, Int, Int)
 *
 * Wait for child processes to stop or terminate
 */
lib7_val_t _lib7_P_Process_waitpid (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int       status, how, val;
    lib7_val_t  r;

    int  pid;

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        pid = waitpid(REC_SELINT(arg, 0), &status, REC_SELWORD(arg, 1));

/*  } while (pid < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or wahtever.	*/

    if (pid < 0)
        return RAISE_SYSERR(lib7_state, pid);

    if (WIFEXITED(status)) {
	how = 0;
	val = WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status)) {
	how = 1;
	val = WTERMSIG(status);
    }
    else if (WIFSTOPPED(status)) {
	how = 2;
	val = WSTOPSIG(status);
    }
    else
        return RAISE_ERROR(lib7_state, "unknown child status");

    REC_ALLOC3(lib7_state, r, INT_CtoLib7(pid), INT_CtoLib7(how), INT_CtoLib7(val));

    return r;

} /* end of _lib7_P_Process_waitpid */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
