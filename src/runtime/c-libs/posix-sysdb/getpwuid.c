/* getpwuid.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"
#include <pwd.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "tags.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_SysDB_getpwuid : word -> String * word * word * String * String
 *
 * Get password file entry by uid.
 */
lib7_val_t _lib7_P_SysDB_getpwuid (lib7_state_t *lib7_state, lib7_val_t arg)
{
    struct passwd*    info;
    lib7_val_t          pw_name, pw_uid, pw_gid, pw_dir, pw_shell, r;

    info = getpwuid(WORD_LIB7toC(arg));
    if (info == NULL)
        return RAISE_SYSERR(lib7_state, -1);
  
    pw_name = LIB7_CString (lib7_state, info->pw_name);
    WORD_ALLOC (lib7_state, pw_uid, (Word_t)(info->pw_uid));
    WORD_ALLOC (lib7_state, pw_gid, (Word_t)(info->pw_gid));
    pw_dir = LIB7_CString (lib7_state, info->pw_dir);
    pw_shell = LIB7_CString (lib7_state, info->pw_shell);

    REC_ALLOC5(lib7_state, r, pw_name, pw_uid, pw_gid, pw_dir, pw_shell);

    return r;

} /* end of _lib7_P_SysDB_getpwuid */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
