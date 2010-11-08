/* sysconf.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "../posix-error/posix-name-val.h"

 /* The following table is generated from all _SC_ values
  * in unistd.h. For most systems, this will include
  * _SC_ARG_MAX
  * _SC_CHILD_MAX
  * _SC_CLK_TCK
  * _SC_JOB_CONTROL
  * _SC_NGROUPS_MAX
  * _SC_OPEN_MAX
  * _SC_SAVED_IDS
  * _SC_STREAM_MAX
  * _SC_TZNAME_MAX
  * _SC_VERSION
  *
  * The full POSIX list is given in section 4.8.1 of Std 1003.1b-1993.
  *
  * The Lib7 string used to look up these values has the same
  * form but without the prefix, e.g., to lookup _SC_ARG_MAX,
  * use sysconf "ARG_MAX"
  */
static name_val_t values[] = {
#include "ml_sysconf.h"
};

#define NUMELMS ((sizeof values)/(sizeof (name_val_t)))

/* _lib7_P_ProcEnv_sysconf : String -> word
 *
 *
 * Get configurable system variables
 */
lib7_val_t _lib7_P_ProcEnv_sysconf (lib7_state_t *lib7_state, lib7_val_t arg)
{
    long	val;
    name_val_t  *attribute;
    lib7_val_t	p;

    attribute = _lib7_posix_nv_lookup (STR_LIB7toC(arg), values, NUMELMS);
    if (!attribute) {
        errno = EINVAL;
        return RAISE_SYSERR(lib7_state, -1);
    }
 
    errno = 0;
    while (((val = sysconf(attribute->val)) == -1) && (errno == EINTR)) {
      errno = 0;
      continue;
    }

    if (val >= 0) {
      WORD_ALLOC (lib7_state, p, val);
      return p;
    }
    else if (errno == 0)
        return RAISE_ERROR(lib7_state, "unsupported POSIX feature");
    else
        return RAISE_SYSERR(lib7_state, -1);

} /* end of _lib7_P_ProcEnv_sysconf */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
