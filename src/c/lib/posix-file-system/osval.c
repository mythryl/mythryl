// osval.c


#include "../../config.h"

#include "system-dependent-unix-stuff.h"

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
#include "heap-tags.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "../posix-error/posix-name-val.h"

// NOTE: The following table MUST be in alphabetical order.
//
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



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_osval   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type:  String -> Int
    //
    // Return the OS-dependent, compile-time constant specified by the string.
    //
    // This fn gets bound as   osval   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

    name_val_t* result  =  _lib7_posix_nv_lookup (HEAP_STRING_AS_C_STRING(arg), values, NUMELMS);

    if (result)   return TAGGED_INT_FROM_C_INT(result->val);
    else          return RAISE_ERROR(task, "system constant not defined");
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

