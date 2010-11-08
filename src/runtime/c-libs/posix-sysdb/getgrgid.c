/* getgrgid.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"
#include <stdio.h>
#include <grp.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "tags.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_SysDB_getgrgid : Unt -> (String, Unt, List( String ))
 *
 * Get group file entry by gid.
 */
lib7_val_t _lib7_P_SysDB_getgrgid (lib7_state_t *lib7_state, lib7_val_t arg)
{
    struct group*     info;

    lib7_val_t          gr_name;
    lib7_val_t          gr_gid;
    lib7_val_t          gr_mem;
    lib7_val_t          result;

    info = getgrgid(WORD_LIB7toC(arg));

    if (info == NULL)
        return RAISE_SYSERR(lib7_state, -1);
  
    gr_name = LIB7_CString (lib7_state, info->gr_name);
    WORD_ALLOC (lib7_state, gr_gid, (Word_t)(info->gr_gid));
    gr_mem = LIB7_CStringList(lib7_state, info->gr_mem);

    REC_ALLOC3(lib7_state, result, gr_name, gr_gid, gr_mem);

    return result;
}

/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
