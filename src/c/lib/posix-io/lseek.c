// lseek.c


#include "../../config.h"

#include "system-dependent-unix-stuff.h"

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
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

    int       fd =  GET_TUPLE_SLOT_AS_INT(arg, 0);
    off_t offset =  GET_TUPLE_SLOT_AS_INT(arg, 1);
    int   whence =  GET_TUPLE_SLOT_AS_INT(arg, 2);

    off_t pos = lseek(fd, offset, whence);

    CHECK_RETURN(task, pos)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

