// sleep.c


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

#include <stdio.h>
#include <string.h>

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif



// One of the library bindings exported via
//     src/c/lib/posix-process/cfun-list.h
// and thence
//     src/c/lib/posix-process/libmythryl-posix-process.c


//    SLEEP(3)                   Linux Programmer's Manual                  SLEEP(3)
//    
//    NAME
//           sleep - Sleep for the specified number of seconds
//    
//    SYNOPSIS
//           #include <unistd.h>
//    
//           unsigned int sleep(unsigned int seconds);
//    
//    DESCRIPTION
//           sleep()  makes  the  calling  thread  sleep  until seconds seconds have
//           elapsed or a signal arrives which is not ignored.
//    
//    RETURN VALUE
//           Zero if the requested time has elapsed, or the number of  seconds  left
//           to sleep, if the call was interrupted by a signal handler.
//    
//    BUGS
//           sleep()  may be implemented using SIGALRM; mixing calls to alarm(2) and
//           sleep() is a bad idea.


Val   _lib7_P_Process_sleep   (Task* task,  Val arg)   {
    //=====================
    //
    // _lib7_P_Process_sleep:   Int -> Int
    //
    // Suspend execution for interval in seconds.
    //
    // This fn gets bound as   sleep'   in:
    //
    //     src/lib/std/src/psx/posix-process.pkg
    //
    // NB: select() allows sleeping to sub-second resolution.

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int seconds = TAGGED_INT_TO_C_INT( arg );

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	int iresult = sleep( seconds );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    Val result = TAGGED_INT_FROM_C_INT( iresult );

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.

