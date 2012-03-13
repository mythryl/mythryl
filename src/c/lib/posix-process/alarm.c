// alarm.c



#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

//       ALARM(2)                   Linux Programmer's Manual                  ALARM(2)
//
//       SYNOPSIS
//              #include <unistd.h>
//
//              unsigned int alarm(unsigned int seconds);
//
//       DESCRIPTION
//	      alarm()  arranges  for  a SIGALRM signal to be delivered to the calling
//	      process in seconds seconds.
//
//	      If seconds is zero, no new alarm() is scheduled.
//
//	      In any event any previously set alarm() is canceled.
//
//       RETURN VALUE
//	      alarm() returns the number of seconds remaining  until  any  previously
//	      scheduled alarm was due to be delivered, or zero if there was no previ-
//	      ously scheduled alarm.


Val   _lib7_P_Process_alarm   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type:   Int -> Int
    //
    // Set a process alarm clock
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_Process_alarm");

    int seconds = TAGGED_INT_TO_C_INT( arg );

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_Process_alarm", NULL );
	//
	int result = alarm( seconds );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_Process_alarm" );

    return TAGGED_INT_FROM_C_INT( result );
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

