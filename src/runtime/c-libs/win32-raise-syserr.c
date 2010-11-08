/* win32-raise-syserr.c
 *
 */

#include "../config.h"

#include <windows.h>
#include "runtime-base.h"
#include "runtime-state.h"
#include "runtime-heap.h"
#include "runtime-globals.h"
#include "lib7-c.h"

/* RaiseSysError:
 *
 * Raise the Lib7 exception SysErr, which has the spec:
 *
 *    exception SYSTEM_ERROR of (String * System_Error Null_Or)
 *
 * We use the last win32-api error value as the System_Error; eventually that
 * will be represented by an (int * String) pair.  If alt_msg is non-zero,
 * then use it as the error string and use NULL for the System_Error.
 */
lib7_val_t RaiseSysError (lib7_state_t *lib7_state, const char *altMsg, char *at)
{
    lib7_val_t	    s, syserror, arg, exn, atStk;
    const char	    *msg;
    char	    buf[32];
    int             errno = -1;

    if (altMsg != NULL) {
	msg = altMsg;
	syserror = OPTION_NONE;
    }
    else {
        errno = (int) GetLastError();
	sprintf(buf, "<win32 error code %d>", errno);
	msg = buf;
	OPTION_SOME(lib7_state, syserror, INT_CtoLib7(errno));
    }

    s = LIB7_CString (lib7_state, msg);
    if (at != NULL) {
	lib7_val_t atMsg = LIB7_CString (lib7_state, at);
	LIST_cons(lib7_state, atStk, atMsg, LIST_nil);
    }
    else
	atStk = LIST_nil;
    REC_ALLOC2 (lib7_state, arg, s, syserror);
    EXN_ALLOC (lib7_state, exn, PTR_CtoLib7(SysErrId), arg, atStk);

    RaiseLib7Exception (lib7_state, exn);

    return exn;

} /* end of RaiseSysError */


/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 */
