// lseek_64.c
//
//   Like lseek.c, but with 64-bit position values.



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
#include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-io/cfun-list.h
// and thence
//     src/c/lib/posix-io/libmythryl-posix-io.c



Val   _lib7_P_IO_lseek_64   (Task* task,  Val arg)   {		// Move read/write file pointer.
    //===================
    //
    // Mythryl type is:
    //
    //      (Int, Unt1, Unt1, Int) -> (Unt1, Unt1)
    //
    // This fn gets bound as   lseek'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-io-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_IO_lseek_64");

    int fd =  GET_TUPLE_SLOT_AS_INT(arg, 0);


    #if (SIZEOF_OFF_T > 4)					// i.e., (sizeof(off_t) > 4)   --   see  src/c/config/generate-sizes-of-some-c-types-h.c
        //
	off_t	offset = ( ((off_t) WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg, 1))) << 32)
			 | ((off_t)(WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg, 2))));
    #else
        off_t   offset = ((off_t)(WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg, 2))));
    #endif


    int  whence = GET_TUPLE_SLOT_AS_INT(arg, 3);


    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_IO_lseek_64", arg );
	//
	off_t pos =  lseek(fd, offset, whence);
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_IO_lseek_64" );

    if (pos < 0)    RAISE_SYSERR (task, (int)pos);

    #if (SIZEOF_OFF_T > 4)						// As above.
        Val poshi =  make_one_word_unt(task,   (Unt1) (pos >> 32));	// 64-bit issue.
    #else
        Val poshi =  make_one_word_unt(task,   (Unt1) 0);
    #endif

    Val     poslo =  make_one_word_unt(task,   (Unt1) pos);

    return  make_two_slot_record(task,  poshi, poslo  );		// What fools these mortals be.
}


// Copyright (c) 2004 by The Fellowship of SML/NJ
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

