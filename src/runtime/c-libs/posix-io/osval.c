/* osval.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "tags.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "../posix-error/posix-name-val.h"

static name_val_t values [] = {
  {"F_GETLK",  F_GETLK},
  {"F_RDLCK",  F_RDLCK},
  {"F_SETLK",  F_SETLK},
  {"F_SETLKW", F_SETLKW},
  {"F_UNLCK",  F_UNLCK},
  {"F_WRLCK",  F_WRLCK},
  {"SEEK_CUR", SEEK_CUR},
  {"SEEK_END", SEEK_END},
  {"SEEK_SET", SEEK_SET},
  {"append",   O_APPEND},
  {"cloexec",  FD_CLOEXEC},
#ifdef O_DSYNC
  {"dsync",    O_DSYNC},
#else
  {"dsync",    0},
#endif
  {"nonblock", O_NONBLOCK},
#ifdef O_RSYNC
  {"rsync",    O_RSYNC},
#else
  {"rsync",    0},
#endif
#ifdef O_SYNC
  {"sync",     O_SYNC},
#else
  {"sync",     0},
#endif
};

#define NUMELMS ((sizeof values)/(sizeof (name_val_t)))

/* _lib7_P_IO_osval : String -> int
 *
 * Return the OS-dependent, compile-time constant specified by the string.
 */
lib7_val_t _lib7_P_IO_osval (lib7_state_t *lib7_state, lib7_val_t arg)
{
    name_val_t      *res;
    
    res = _lib7_posix_nv_lookup (STR_LIB7toC(arg), values, NUMELMS);
    if (res)
	return INT_CtoLib7(res->val);
    else {
      return RAISE_ERROR(lib7_state, "system constant not defined");
    }

} /* end of _lib7_P_IO_osval */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
