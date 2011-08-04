// utime.c


#include "../../config.h"

#include "system-dependent-unix-stuff.h"

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#if HAVE_UTIME_H
    #include <utime.h>
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



Val   _lib7_P_FileSys_utime   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type: (String, int32::Int, int32::Int) -> Void
    //                name    actime      modtime
    //
    // Sets file access and modification times.
    // If actime = -1, then set both to current time.
    //
    // This fn gets bound as   utime'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-filesys-64.pkg

    Val	    path    =  GET_TUPLE_SLOT_AS_VAL(     arg, 0);
    time_t  actime  =  TUPLE_GET_INT32(arg, 1);
    time_t  modtime =  TUPLE_GET_INT32(arg, 2);

    int status;

    if (actime == -1) {
        status = utime (HEAP_STRING_AS_C_STRING(path), NULL);
    } else {
	struct utimbuf tb;

	tb.actime = actime;
	tb.modtime = modtime;
// printf("src/c/lib/posix-filesys/utime.c calling utime(%s)...\n",HEAP_STRING_AS_C_STRING(path));
	status = utime (HEAP_STRING_AS_C_STRING(path), &tb);
    }

    CHECK_RETURN_UNIT(task, status)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

