/* errmsg.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "tags.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_Error_errmsg : Int -> String
 *
 * Return the OS-dependent error message associated with error.
 */
lib7_val_t _lib7_P_Error_errmsg (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		    errnum = INT_LIB7toC(arg);
    lib7_val_t	    s;

#if defined(HAS_STRERROR)
    char	    *msg = strerror(errnum);
    if (msg != 0)
	s = LIB7_CString (lib7_state, msg);
    else {
	char		buf[64];
	sprintf(buf, "<unknown error %d>", errnum);
	s = LIB7_CString (lib7_state, buf);
    }
#else
    if ((0 <= errnum) && (errnum < sys_nerr))
	s = LIB7_CString (lib7_state, sys_errlist[errnum]);
    else {
	char		buf[64];
	sprintf(buf, "<unknown error %d>", errnum);
	s = LIB7_CString (lib7_state, buf);
    }
#endif

    return s;

} /* end of _lib7_P_Error_errmsg */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
