/* readlink.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif

#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"


/* _lib7_P_FileSys_readlink : String -> String
 *
 * Read the value of a symbolic link.
 *
 * The following implementation assumes that the system readlink
 * fills the given buffer as much as possible, without nul-termination,
 * and returns the number of bytes copied. If the buffer is not large
 * enough, the return value will be at least the buffer size. In that
 * case, we find out how big the link really is, allocate a buffer to
 * hold it, and redo the readlink.
 *
 * Note that the above semantics are not those of POSIX, which requires
 * null-termination on success, and only fills the buffer up to at most 
 * the penultimate byte even on failure.
 *
 * Should this be written to avoid the extra copy, using heap memory?
 */
lib7_val_t _lib7_P_FileSys_readlink (lib7_state_t *lib7_state, lib7_val_t arg)
{
    char        *path = STR_LIB7toC(arg);
    char	buf[MAXPATHLEN];
    int         len;

    len = readlink(path, buf, MAXPATHLEN);

    if (len < 0)
        return RAISE_SYSERR(lib7_state, len);
    else if (len < MAXPATHLEN) {
	buf[len] = '\0';
	return LIB7_CString (lib7_state, buf);
    }
    else {  /* buffer not big enough */
	char         *nbuf;
	lib7_val_t     chunk;
	struct stat  sbuf;
	int          res;
	int          nlen;

      /* Determine how big the link text is and allocate a buffer */
	res = lstat (path, &sbuf);
	if (res < 0)
  	    return RAISE_SYSERR(lib7_state, res);
	nlen = sbuf.st_size + 1;
	nbuf = MALLOC(nlen);
	if (nbuf == 0)
	  return RAISE_ERROR(lib7_state, "out of malloc memory");

        /* Try the readlink again. Give up on error or if len is still bigger
         * than the buffer size.
         */
	len = readlink(path, buf, len);
	if (len < 0)
	    return RAISE_SYSERR(lib7_state, len);
	else if (len >= nlen)
	  return RAISE_ERROR(lib7_state, "readlink failure");

	nbuf[len] = '\0';
	chunk = LIB7_CString (lib7_state, nbuf);
	FREE (nbuf);
	return chunk;
    }

} /* end of _lib7_P_FileSys_readlink */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
