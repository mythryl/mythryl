// exit.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif


#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-os/cfun-list.h
// and thence
//     src/c/lib/posix-os/libmythryl-posix-os.c



Val   _lib7_OS_tmpname   (Task* task,  Val arg)   {
    //================
    //
    // Generate a unique name for a temporary file.
    //
    // Mythryl type:   Void -> String
    //
    // This fn gets bound as   tmp_name   in:
    //
    //     src/lib/std/src/posix/winix-file.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    static int call_number = 0;
    static int pid         = 0;

    char buf[ 132 ];
    
    int c1 = ++call_number;			// Try to make our filename unique.
    
    if (!pid) {
	RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	    //
	    pid = getpid();				// Try to harder to make our filename unique.
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );
    }

    int c2 = ++call_number;			// Try to harder yet to make our filename unique. :-)
 
    sprintf (buf, "tmpfile.%d.%d.%d.tmp", c1, pid, c2);
    //
    Val result = make_ascii_string_from_c_string__may_heapclean (task, buf, NULL);

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}
//
// NOTES:
//     Here we used to use tmpnam(), but that generated warnings that it is dangerous,
//     and that it is better to use mkstemp()  -- but the mkstemp() manpage says:
//
//        "Don't use this function, use tmpfile(3) instead.
//         It is better  defined and more portable."
//    
//     They seem to mainly be worried about malicious processes guessing the tmpfile
//     name and cracking security by overwriting the file.  (Personally, I'm not too
//     worried about this, I figure that if malicious agents control a process on
//     my machine, I'm already toast -- and anyhow we're no longer living in the old
//     world of campus timesharing Unix machines!)
//
//     tmpfile() doesn't fit Mythryl very well because it returns a FILE*,
//     and as a rule we don't use C buffered I/O in Mythryl.
//
//     mkstemp() returns an fd, which fits Mythryl better,
//     so we now provide it via
//
//         src/c/lib/posix-file-system/mkstemp.c
//
//     But it is no use if we're generating a bash commandline line to execute,
//     so we continue to provide this function.
//
//         -- 2011-10-08 CrT

// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

