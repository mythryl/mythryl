/* getgroups.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>

#if HAVE_LIMITS_H
#include <limits.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* some OSs use int[] as the second argument to getgroups(), when gid_t
 * is not int.
 */
#ifdef INT_GIDLIST
typedef int  gid;
#else
typedef gid_t gid;
#endif


/* mkList:
 *
 * Convert array of gid_t into a list of gid_t
 */
static lib7_val_t mkList (lib7_state_t *lib7_state, int ngrps, gid gidset[])
{
    lib7_val_t    p, w;

/** NOTE: we should do something about possible GC!!! XXX BUGGO FIXME **/

    p = LIST_nil;
    while (ngrps-- > 0) {
        WORD_ALLOC (lib7_state, w, (Word_t)(gidset[ngrps]));
	LIST_cons(lib7_state, p, w, p);
    }

    return p;
}

/* _lib7_P_ProcEnv_getgroups: Void -> word list
 *
 * Return supplementary group access list ids.
 */
lib7_val_t _lib7_P_ProcEnv_getgroups (lib7_state_t *lib7_state, lib7_val_t arg)
{
    gid		gidset[NGROUPS_MAX];
    int		ngrps;
    lib7_val_t	p;

    ngrps = getgroups (NGROUPS_MAX, gidset);

    if (ngrps == -1) {
	gid      *gp;

      /* If the error was not due to too small buffer size,
       * raise exception.
       */
	if (errno != EINVAL)
	    return RAISE_SYSERR(lib7_state, -1);

      /* Find out how many groups there are and allocate enough space. */
	ngrps = getgroups (0, gidset);
	gp = (gid *)MALLOC(ngrps * (sizeof (gid)));
	if (gp == 0) {
	    errno = ENOMEM;
	    return RAISE_SYSERR(lib7_state, -1);
	}

	ngrps = getgroups (ngrps, gp);

	if (ngrps == -1)
	  p = RAISE_SYSERR(lib7_state, -1);
	else
	    p = mkList (lib7_state, ngrps, gp);
        
	FREE ((void *)gp);
    }
    else
	p = mkList (lib7_state, ngrps, gidset);
    
    return p;

} /* end of _lib7_P_ProcEnv_getgroups */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
