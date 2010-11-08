/* getcwd.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif

#include <errno.h>

#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif


/* _lib7_P_FileSys_getcwd : Void -> String
 *
 * Get current working directory pathname
 *
 * Should this be written to avoid the extra copy?
 */
lib7_val_t _lib7_P_FileSys_getcwd (lib7_state_t *lib7_state, lib7_val_t arg)
{
    char	path[MAXPATHLEN];
    char*	status;
    lib7_val_t    p;
    int         buflen;
    char        *buf;

    status = getcwd(path, MAXPATHLEN);

    if (status != NULL)
	return LIB7_CString (lib7_state, path);

    if (errno != ERANGE)
        return RAISE_SYSERR(lib7_state, status);

    buflen = 2*MAXPATHLEN;
    buf = MALLOC(buflen);
    if (buf == NULL)
      return RAISE_ERROR(lib7_state, "no malloc memory");

    while ((status = getcwd(buf, buflen)) == NULL) {
        FREE (buf);
        if (errno != ERANGE) {

	    return RAISE_SYSERR(lib7_state, status);

        } else {

            buflen = 2*buflen;
            buf = MALLOC(buflen);
            if (buf == NULL)
	      return RAISE_ERROR(lib7_state, "no malloc memory");
        }
    }
      
    p = LIB7_CString (lib7_state, buf);
    FREE (buf);
      
    return p;

} /* end of _lib7_P_FileSys_getcwd */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
