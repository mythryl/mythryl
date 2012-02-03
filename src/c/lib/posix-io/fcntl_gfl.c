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
    #include "lib7-c.h"
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
    //     src/lib/std/src/posix-1003.1b/posix-io.pkg
    //     src/lib/std/src/posix-1003.1b/posix-io-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_IO_fcntl_gfl");

    int fd = TAGGED_INT_TO_C_INT(arg);

    int flag;
    Val flags;
    Val mode;

/*  do { */						// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_IO_fcntl_gfl", arg );
	    //
	    flag = fcntl(fd, F_GETFD);
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_IO_fcntl_gfl" );

/*  } while (flag < 0 && errno == EINTR);	*/	// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    if (flag < 0)   return  RAISE_SYSERR(task, flag );

    flags =  make_one_word_unt(task,  (flag & (~O_ACCMODE)) );
    mode  =  make_one_word_unt(task,  (flag &   O_ACCMODE)  );

    return  make_two_slot_record(task,  flags, mode  );
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

