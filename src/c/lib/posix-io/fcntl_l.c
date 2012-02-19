// fcntl_l.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "system-dependent-unix-stuff.h"

#if HAVE_FCNTL_H
    #include <fcntl.h>
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



Val   _lib7_P_IO_fcntl_l   (Task* task,  Val arg)   {
    //==================
    //
    // Mythryl type:   (Int, Int, Flock_Rep) -> Flock_Rep
    //    Flock_Rep = (Int, Int, Offset, Offset, Int)
    //
    // Handle record locking.
    //
    // This fn gets bound as   fcntl_l   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-io.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_IO_fcntl_l");

    int fd        =  GET_TUPLE_SLOT_AS_INT( arg, 0 );
    int cmd       =  GET_TUPLE_SLOT_AS_INT( arg, 1 );
    Val flock_rep =  GET_TUPLE_SLOT_AS_VAL( arg, 2 );

    struct flock     flock;
    int              status;
    
    flock.l_type = GET_TUPLE_SLOT_AS_INT(flock_rep, 0);
    flock.l_whence = GET_TUPLE_SLOT_AS_INT(flock_rep, 1);
    flock.l_start = GET_TUPLE_SLOT_AS_INT(flock_rep, 2);
    flock.l_len = GET_TUPLE_SLOT_AS_INT(flock_rep, 3);
   
/*  do { */						// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_IO_fcntl_l", &arg );
	    //
	    status = fcntl(fd, cmd, &flock);
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_IO_fcntl_l" );

/*  } while (status < 0 && errno == EINTR);	*/	// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

    if (status < 0)   return RAISE_SYSERR__MAY_HEAPCLEAN(task, status, NULL);

    return  make_five_slot_record( task,
		//
		TAGGED_INT_FROM_C_INT( flock.l_type ),
		TAGGED_INT_FROM_C_INT( flock.l_whence ), 
		TAGGED_INT_FROM_C_INT( flock.l_start ),
		TAGGED_INT_FROM_C_INT( flock.l_len ),
		TAGGED_INT_FROM_C_INT( flock.l_pid )
	    );
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

