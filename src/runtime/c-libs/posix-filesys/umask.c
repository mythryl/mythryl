/* umask.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_FileSys_umask : word -> word
 *
 * Set and get file creation mask
 * Assumes umask never fails.
 */
lib7_val_t _lib7_P_FileSys_umask (lib7_state_t *lib7_state, lib7_val_t arg)
{
    mode_t		omask;
    lib7_val_t            p;

    omask = umask(WORD_LIB7toC(arg));
    WORD_ALLOC (lib7_state, p, (Word_t)omask);

    return p;

} /* end of _lib7_P_FileSys_umask */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
