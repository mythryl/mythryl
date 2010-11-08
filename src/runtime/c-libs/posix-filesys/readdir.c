/* readdir.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#endif

#include <errno.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "tags.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_FileSys_readdir : chunk -> String
 *
 * Return the next filename from the directory stream.
 */
lib7_val_t _lib7_P_FileSys_readdir (lib7_state_t *lib7_state, lib7_val_t arg)
{
    struct dirent      *dirent;
    
    while (TRUE) {
	errno = 0;

	dirent = readdir(PTR_LIB7toC(DIR, arg));

	if (dirent == NULL) {
	    if (errno != 0)     /* Error occurred */
	        return RAISE_SYSERR(lib7_state, -1);
	    else                /* End of stream */
		return LIB7_string0;
	}
	else {
	    char	*cp = dirent->d_name;

 /* SML/NJ drops "." and ".." at this point,
           but that is alien to posix culture,
           so I've commented it out:			-- 2008-02-23 CrT
 */
 /*	    if ((cp[0] == '.')
	    && ((cp[1] == '\0') || ((cp[1] == '.') && (cp[2] == '\0'))))
		continue;
	    else
 */
		return LIB7_CString (lib7_state, cp);
	}
    }

} /* end of _lib7_P_FileSys_readdir */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
