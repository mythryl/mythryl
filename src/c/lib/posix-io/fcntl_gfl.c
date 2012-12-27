// fcntl_gfl.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "system-dependent-unix-stuff.h"

#if HAVE_FCNTL_H
    #include <fcntl.h>
#endif

#include "make-strings-and-vectors-etc.h"
    #include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-io/cfun-list.h
// and thence
//     src/c/lib/posix-io/libmythryl-posix-io.c



Val   _lib7_P_IO_fcntl_gfl   (Task* task,  Val arg)   {
    //====================
    //
    // Mythryl type:   Int -> (Unt, Unt)
    //
    // Get file status flags and file access modes.
    //
    // This fn gets bound as   fcntl_gfl   in:
    //
    //     src/lib/std/src/psx/posix-io.pkg
    //     src/lib/std/src/psx/posix-io-64.pkg

										ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int fd = TAGGED_INT_TO_C_INT( arg );

    int flag;
    Val flags;
    Val mode;

    do {
	//
	RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	    //
	    flag = fcntl(fd, F_GETFL);						// SML/NJ has F_GETFD here...?
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );
	//
    } while (flag < 0 && errno == EINTR);					// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    if (flag < 0)   return  RAISE_SYSERR__MAY_HEAPCLEAN(task, flag, NULL);

    flags =  make_one_word_unt(task,  (flag & (~O_ACCMODE)) );
    mode  =  make_one_word_unt(task,  (flag &   O_ACCMODE)  );

    Val result =  make_two_slot_record(task,  flags, mode  );
										EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

