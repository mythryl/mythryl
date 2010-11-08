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

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "tags.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "../posix-error/posix-name-val.h"

/* NOTE: the following table must be in alphabetical order!!! */
static name_val_t values [] = {
  {"FILE_EXISTS",  F_OK},
  {"MAY_EXECUTE",  X_OK},
  {"MAY_READ",     R_OK},
  {"MAY_WRITE",    W_OK},
  {"O_APPEND",     O_APPEND},
  {"O_CREAT",      O_CREAT},
#ifdef O_DSYNC
  {"O_DSYNC",      O_DSYNC},
#else
  {"O_DSYNC",      0},
#endif
  {"O_EXCL",       O_EXCL},
  {"O_NOCTTY",     O_NOCTTY},
  {"O_NONBLOCK",   O_NONBLOCK},
  {"O_RDONLY",     O_RDONLY},
  {"O_RDWR",       O_RDWR},
#ifdef O_RSYNC
  {"O_RSYNC",      O_RSYNC},
#else
  {"O_RSYNC",      0},
#endif
#ifdef O_SYNC
  {"O_SYNC",       O_SYNC},
#else
  {"O_SYNC",       0},
#endif
  {"O_TRUNC",      O_TRUNC},
  {"O_WRONLY",     O_WRONLY},
  {"irgrp",        S_IRGRP},
  {"iroth",        S_IROTH},
  {"irusr",        S_IRUSR},
  {"irwxg",        S_IRWXG},
  {"irwxo",        S_IRWXO},
  {"irwxu",        S_IRWXU},
  {"isgid",        S_ISGID},
  {"isuid",        S_ISUID},
  {"iwgrp",        S_IWGRP},
  {"iwoth",        S_IWOTH},
  {"iwusr",        S_IWUSR},
  {"ixgrp",        S_IXGRP},
  {"ixoth",        S_IXOTH},
  {"ixusr",        S_IXUSR},
};

#define NUMELMS ((sizeof values)/(sizeof (name_val_t)))

/* _lib7_P_FileSys_osval : String -> int
 *
 * Return the OS-dependent, compile-time constant specified by the string.
 */
lib7_val_t _lib7_P_FileSys_osval (lib7_state_t *lib7_state, lib7_val_t arg)
{
    name_val_t  *result  =  _lib7_posix_nv_lookup (STR_LIB7toC(arg), values, NUMELMS);

    if (result) {
	return INT_CtoLib7(result->val);
    }

    return RAISE_ERROR(lib7_state, "system constant not defined");
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
