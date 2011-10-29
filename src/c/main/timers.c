// timers.c
//
// OS independent timer routines.
//
// These rely on a OS dependent implementation
// of the following function:
//
//	void get_cpu_time (Time *user_t, Time *sys_t);

#include "../mythryl-config.h"

#include "runtime-base.h"
#include "pthread-state.h"
#include "runtime-timer.h"


void   reset_timers   (Pthread* pthread)   {
    //
    // Clear the cleaner timers.
    //
    pthread->cumulative_cleaning_cpu_time->seconds = 0;
    pthread->cumulative_cleaning_cpu_time->uSeconds = 0;
}


void   start_cleaning_timer   (Pthread* pthread)   {
    //
    get_cpu_time( pthread->cpu_time_at_start_of_last_cleaning,  NULL );
}


void   stop_cleaning_timer   (Pthread* pthread,  long* time) {
    //
    // Stop the cleaning timer and update
    // the cumulative cleaning time.
    // If time is not NULL, then return the time (in ms.)
    // spent since the start of the cleaning.   (garbage collection)

    int   sec;
    int   usec;
    Time  t1;
    Time* gt0 = pthread->cpu_time_at_start_of_last_cleaning;
    Time* gt  = pthread->cumulative_cleaning_cpu_time;


    get_cpu_time( &t1, NULL );

    sec  = t1.seconds  - gt0->seconds;
    usec = t1.uSeconds - gt0->uSeconds;

    if (time != NULL) {
        //
	if (usec < 0) {
	    sec--; usec += 1000000;
	} else if (usec > 1000000) {
	    sec++; usec -= 1000000;
	}
	*time = (usec/1000 + sec*1000);
    }

    sec = gt->seconds + sec;
    usec = gt->uSeconds + usec;
    if (usec < 0) {
	sec--; usec += 1000000;
    } else if (usec > 1000000) {
	sec++; usec -= 1000000;
    }
    gt->seconds = sec;
    gt->uSeconds = usec;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


