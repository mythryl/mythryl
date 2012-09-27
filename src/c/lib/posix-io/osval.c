// osval.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
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


// One of the library bindings exported via
//     src/c/lib/posix-io/cfun-list.h
// and thence
//     src/c/lib/posix-io/libmythryl-posix-io.c



Val   _lib7_P_IO_osval   (Task* task,  Val arg)   {
    //================
    //
    // Mythryl type:   String -> Sy_Int
    //
    // Return the OS-dependent, compile-time constant specified by the string.
    //
    // This fn gets bound as   close'   in:
    //
    //     src/lib/std/src/psx/posix-io.pkg
    //     src/lib/std/src/psx/posix-io-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_IO_osval");

    name_val_t* result = _lib7_posix_nv_lookup (HEAP_STRING_AS_C_STRING(arg), values, NUMELMS);

    if (result)   return TAGGED_INT_FROM_C_INT(result->val);
    else          return RAISE_ERROR__MAY_HEAPCLEAN(task, "system constant not defined", NULL);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

