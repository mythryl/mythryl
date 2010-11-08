/* unix-raise-syserr.c
 *
 */

#include "../config.h"

#include "runtime-unixdep.h"
#ifdef HAS_STRERROR
#  include <string.h>
#endif
#include <stdio.h>
#include <errno.h>
#include "runtime-base.h"
#include "runtime-state.h"
#include "runtime-heap.h"
#include "runtime-globals.h"
#include "lib7-c.h"


#ifndef HAS_STRERROR
/* strerror:
 * An implementation of strerror for those systems that do not provide it.
 */
static char *strerror (int errnum)
{
    extern int	sys_nerr;
    extern char	*sys_errlist[];

    if ((errnum < 0) || (sys_nerr <= errnum))
	return "<unknown system error>";
    else
	return sys_errlist[errnum];

} /* end of strerror */
#endif


/* RaiseSysError:
 *
 * Raise the Lib7 exception SYSTEM_ERROR, which has the spec:
 *
 *    exception SYSTEM_ERROR (String, Null_Or(System_Error) );
 *
 * We normally get invoked via either the
 * RAISE_SYSERR or RAISE_ERROR macro from
 *
 *     src/runtime/c-libs/lib7-c.h 
 *
 * For the time being, we use the errno value as the System_Error; eventually that
 * will be represented by an (Int, String) pair.  If alt_msg is non-zero,
 * then use it as the error string and use NULL for the System_Error.
 */
lib7_val_t  RaiseSysError (

    lib7_state_t*  lib7_state,
    const char*    altMsg,
    const char*    at			/* C sourcefile and line number raising this error:  "<foo.c:37>"	*/

) {
    int error_number = errno;		/* Various calls can trash this value so preserve it early.		*/

    lib7_val_t	    errno_string;	/* From strerror(errno).						*/
    lib7_val_t	    at_list;		/* [] or [ "<foo.c:187>" ].						*/
    lib7_val_t	    null_or_errno;	/* NULL or (THE errno).							*/
    lib7_val_t	    arg;		/* Holds pair (errno_string, null_or_errno).				*/
    lib7_val_t	    syserr_exception;	/* Final return value and exception raised.				*/

    const char*	    msg;
    char	    buf[32];

    if (altMsg != NULL) {

	msg           =  altMsg;
	null_or_errno =  OPTION_NONE;

    } else if ((msg = strerror(error_number)) != NULL) {

	OPTION_SOME (lib7_state, null_or_errno, INT_CtoLib7(error_number))

    } else {

	sprintf(buf, "<unknown error %d>", error_number);
	msg = buf;
	OPTION_SOME(lib7_state, null_or_errno, INT_CtoLib7(error_number));
    }

#if (defined(DEBUG_OS_INTERFACE) || defined(DEBUG_TRACE_CCALL))
    SayDebug ("RaiseSysError: errno = %d, msg = \"%s\"\n",
	(altMsg != NULL) ? -1 : error_number, msg);
#endif

    errno_string
	=
	LIB7_CString (lib7_state, msg);

    if (at != NULL) {

	lib7_val_t  at_cstring
            =
            LIB7_CString (lib7_state, at);

	LIST_cons(lib7_state, at_list, at_cstring, LIST_nil);

    } else {

	at_list = LIST_nil;
    }

    REC_ALLOC2 (lib7_state, arg, errno_string, null_or_errno);
    EXN_ALLOC  (lib7_state, syserr_exception, PTR_CtoLib7(SysErrId), arg, at_list);

    /* Modify the Lib7 state so that 'syserr_exception'
     * will be raised when Mythryl execution resumes:
     */
    RaiseLib7Exception (lib7_state, syserr_exception);		/* RaiseLib7Exception	is from    src/runtime/main/run-runtime.c	*/

    return  syserr_exception;

} /* end of RaiseSysError */


/*
 * COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 */
