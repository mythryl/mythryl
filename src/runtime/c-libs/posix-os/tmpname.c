/* exit.c
 *
 */

#include "../../config.h"

#include <stdio.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"

/* XXX BUGGO FIXME:
 * tmpname.c:(.text+0xe): warning: the use of `tmpnam' is dangerous, better use `mkstemp'
 * -- but the mkstemp() manpage says:
 *
 *      "Don't use this function, use tmpfile(3) instead.
 *       It is better  defined and more portable."
 */

/* _lib7_OS_tmpname:
 */
lib7_val_t _lib7_OS_tmpname (lib7_state_t *lib7_state, lib7_val_t arg)
{
    char	buf[L_tmpnam];

    tmpnam (buf);

    return LIB7_CString (lib7_state, buf);

} /* end of _lib7_OS_tmpname */


/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
