// lseek.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#include <stdio.h>
#include <string.h>

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-io/cfun-list.h
// and thence
//     src/c/lib/posix-io/libmythryl-posix-io.c



Val   _lib7_P_IO_lseek   (Task* task,  Val arg)   {
    //================
    //
    // Mythryl type:  (Int, Int, Int) -> Int
    //
    // Move read/write file pointer.
    //
    // This fn gets bound as   lseek'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-io.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_IO_lseek");

    int       fd =  GET_TUPLE_SLOT_AS_INT( arg, 0 );
    off_t offset =  GET_TUPLE_SLOT_AS_INT( arg, 1 );
    int   whence =  GET_TUPLE_SLOT_AS_INT( arg, 2 );

    RELEASE_MYTHRYL_HEAP( task->pthread, "", NULL );
	//
	off_t pos = lseek(fd, offset, whence);
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "" );

    RETURN_STATUS_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, pos, NULL);
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

