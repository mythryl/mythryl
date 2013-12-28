// errno-sysconsts-table.c
//
// The table of system constants representing the Posix error codes.


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include <errno.h>

#ifndef EBADMSG
#define EBADMSG 0
#endif

#ifndef ECANCELED
#define ECANCELED 0
#endif

#ifndef ENOTSUP
#define ENOTSUP 0
#endif

// This gets bound as   errors   in:
//
//    src/lib/std/src/psx/posix-error.pkg

static System_Constant   errno_sysconsts_table_guts[] = {
    //                   ==========================
    //
    {EACCES,		"acces"},
    {EAGAIN,		"again"},
    //
    {EBADF,		"badf"},
    //
    {EBADMSG,		"badmsg"},
    //
    {EBUSY,		"busy"},
    //
    {ECANCELED,		"canceled"},
    //
    {ECHILD,		"child"},
    {EDEADLK,		"deadlk"},
    {EDOM,		"dom"},
    //
    {EEXIST,		"exist"},
    {EFAULT,		"fault"},
    //
    {EFBIG,		"fbig"},
    {EINPROGRESS,	"inprogress"},
    {EINTR,		"intr"},
    //
    {EINVAL,		"inval"},
    {EIO,		"io"},
    {EISDIR,		"isdir"},
    //
    {ELOOP,		"loop"},
    {EMFILE,		"mfile"},
    {EMLINK,		"mlink"},
    //
    {EMSGSIZE,		"msgsize"},
    {ENAMETOOLONG,	"nametoolong"},
    {ENFILE,		"nfile"},
    //
    {ENODEV,		"nodev"},
    {ENOENT,		"noent"},
    {ENOEXEC,		"noexec"},
    //
    {ENOLCK,		"nolck"},
    {ENOMEM,		"nomem"},
    {ENOSPC,		"nospc"},
    //
    {ENOSYS,		"nosys"},
    {ENOTDIR,		"notdir"},
    {ENOTEMPTY,		"notempty"},
    {ENOTSUP,		"notsup"},
    {ENOTTY,		"notty"},
    {ENXIO,		"nxio"},
    //
    {EPERM,		"perm"},
    {EPIPE,		"pipe"},
    //
    {ERANGE,		"range"},
    {EROFS,		"rofs"},
    {ESPIPE,		"spipe"},
    //
    {ESRCH,		"srch"},
    {E2BIG,		"toobig"},
    {EXDEV,		"xdev"},
    //
#if (defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN))
    {EWOULDBLOCK,	"wouldblock"},
#endif
};

Sysconsts   errno_sysconsts_table__global = {
    //      =============================
    //
    sizeof(errno_sysconsts_table_guts) / sizeof(System_Constant),
    errno_sysconsts_table_guts
};


// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

