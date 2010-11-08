/* opendir.c
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

#include "runtime-base.h"
#include "runtime-values.h"
#include "tags.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_FileSys_opendir : String -> chunk
 *
 * Open and return a directory stream.
 */
lib7_val_t _lib7_P_FileSys_opendir (lib7_state_t *lib7_state, lib7_val_t arg)
{
    DIR      *dir;
    
    dir = opendir(STR_LIB7toC(arg));

    if (dir == NULL)  return RAISE_SYSERR(lib7_state, -1);
    else	      return PTR_CtoLib7(dir);

} /* end of _lib7_P_FileSys_opendir */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
