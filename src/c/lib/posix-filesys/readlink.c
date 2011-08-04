// readlink.c


#include "../../config.h"

#include "system-dependent-unix-stuff.h"

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

#if HAVE_SYS_STAT_H
    #include <sys/stat.h>
#endif

#if HAVE_LIMITS_H
    #include <limits.h>
#endif

#if HAVE_SYS_PARAM_H
    #include <sys/param.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-filesys/cfun-list.h
// and thence
//     src/c/lib/posix-filesys/libmythryl-posix-filesys.c



Val   _lib7_P_FileSys_readlink   (Task* task,  Val arg)   {
    //========================
    //
    // Mythryl type:  String -> String
    //
    // Read the value of a symbolic link.
    //
    // The following implementation assumes that the system readlink
    // fills the given buffer as much as possible, without nul-termination,
    // and returns the number of bytes copied. If the buffer is not large
    // enough, the return value will be at least the buffer size. In that
    // case, we find out how big the link really is, allocate a buffer to
    // hold it, and redo the readlink.
    //
    // Note that the above semantics are not those of POSIX, which requires
    // null-termination on success, and only fills the buffer up to at most 
    // the penultimate byte even on failure.
    //
    // Should this be written to avoid the extra copy, using heap memory?
    //
    // This fn gets bound as   readlink   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-filesys-64.pkg

    char* path = HEAP_STRING_AS_C_STRING(arg);
    char  buf[MAXPATHLEN];

    int len = readlink(path, buf, MAXPATHLEN);

    if (len < 0)   return RAISE_SYSERR(task, len);

    if (len < MAXPATHLEN) {
        //
	buf[len] = '\0';
	return make_ascii_string_from_c_string (task, buf);
    }


    // Buffer not big enough.

    // Determine how big the link text is and allocate a buffer.

    struct stat                sbuf;
    int result = lstat (path, &sbuf);

    if (result < 0)   return RAISE_SYSERR(task, result);

    int nlen = sbuf.st_size + 1;

    char* nbuf = MALLOC(nlen);

    if (nbuf == 0)   return RAISE_ERROR(task, "out of malloc memory");

    // Try the readlink again. Give up on error or if len is still bigger
    // than the buffer size.
    //
    len = readlink(path, buf, len);

    if (len < 0)		return RAISE_SYSERR(task, len);
    else if (len >= nlen)	return RAISE_ERROR(task, "readlink failure");

    nbuf[len] = '\0';
    Val chunk = make_ascii_string_from_c_string (task, nbuf);
    FREE (nbuf);
    //
    return chunk;

}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

