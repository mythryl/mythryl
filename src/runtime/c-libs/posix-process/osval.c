/* osval.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "tags.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "../posix-error/posix-name-val.h"

static name_val_t values [] = {
  {"WNOHANG",       WNOHANG},
#ifdef WUNTRACED
  {"WUNTRACED",     WUNTRACED},
#endif
};

#define NUMELMS ((sizeof values)/(sizeof (name_val_t)))

/* _lib7_P_Process_osval : String -> int
 *
 * Return the OS-dependent, compile-time constant specified by the string.
 */
lib7_val_t _lib7_P_Process_osval (lib7_state_t *lib7_state, lib7_val_t arg)
{
    name_val_t *result = _lib7_posix_nv_lookup (STR_LIB7toC(arg), values, NUMELMS);

    if (result)   return INT_CtoLib7(result->val);
    else          return RAISE_ERROR(lib7_state, "system constant not defined");
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
