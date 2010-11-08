/* uname.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"
#include <sys/utsname.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_ProcEnv_uname: Void -> (String * String) list
 *
 * Return names of current system.
 */
lib7_val_t _lib7_P_ProcEnv_uname (lib7_state_t *lib7_state, lib7_val_t arg)
{
    struct utsname      name;
    int                 status;
    lib7_val_t            l, p, s;
    lib7_val_t		field;

    status = uname (&name);

    if (status == -1)
        RAISE_SYSERR(lib7_state, status);

/** NOTE: we should do something about possible GC!!! XXX BUGGO FIXME **/

    l = LIST_nil;

    field = LIB7_CString(lib7_state, "machine");
    s = LIB7_CString(lib7_state, name.machine);
    REC_ALLOC2(lib7_state, p, field, s);
    LIST_cons(lib7_state, l, p, l);

    field = LIB7_CString(lib7_state, "version");
    s = LIB7_CString(lib7_state, name.version);
    REC_ALLOC2(lib7_state, p, field, s);
    LIST_cons(lib7_state, l, p, l);

    field = LIB7_CString(lib7_state, "release");
    s = LIB7_CString(lib7_state, name.release);
    REC_ALLOC2(lib7_state, p, field, s);
    LIST_cons(lib7_state, l, p, l);

    field = LIB7_CString(lib7_state, "nodename");
    s = LIB7_CString(lib7_state, name.nodename);
    REC_ALLOC2(lib7_state, p, field, s);
    LIST_cons(lib7_state, l, p, l);

    field = LIB7_CString(lib7_state, "sysname");
    s = LIB7_CString(lib7_state, name.sysname);
    REC_ALLOC2(lib7_state, p, field, s);
    LIST_cons(lib7_state, l, p, l);

    return l;

} /* end of _lib7_P_ProcEnv_uname */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
