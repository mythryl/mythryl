/* utime.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_UTIME_H
#include <utime.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_FileSys_utime : (String * int32.Int * int32.Int) -> Void
 *                        name     actime modtime
 *
 * Sets file access and modification times. If
 * actime = -1, then set both to current time.
 */
lib7_val_t _lib7_P_FileSys_utime (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	    path = REC_SEL(arg, 0);
    time_t          actime = REC_SELINT32(arg, 1);
    time_t          modtime = REC_SELINT32(arg, 2);
    int		    status;

    if (actime == -1) {
      status = utime (STR_LIB7toC(path), NULL);
    }
    else {
      struct utimbuf tb;

      tb.actime = actime;
      tb.modtime = modtime;
/*printf("src/runtime/c-libs/posix-filesys/utime.c calling utime(%s)...\n",STR_LIB7toC(path));*/
      status = utime (STR_LIB7toC(path), &tb);
    }

    CHECK_RETURN_UNIT(lib7_state, status)

} /* end of _lib7_P_FileSys_utime */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
