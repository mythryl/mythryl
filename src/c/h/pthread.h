// pthread.h
//
// State of a 'Pthread', the Mythryl wrapper for a
// posix thread sharing access to the Mythryl heap.



/*
###                "Light is the task when many share the toil."
###
###                                 -- Homer, circa 750BC
*/



#ifndef PTHREAD_H
#define PTHREAD_H

#include "runtime-base.h"
#include "system-dependent-signal-stuff.h"
#include "system-signals.h"
#include "runtime-timer.h"


#if defined(MULTICORE_SUPPORT)
    #include "runtime-multicore.h"
#endif


// The Pthread state vector:
//
struct pthread {
    //
    Heap* heap;		  					// The heap for this Mythryl task.
								// 'Heap' is defined in	  src/c/h/runtime-base.h

    Task* task;							// The state of the Mythryl task that is
				        			// running on this Pthread.  Eventually	
				        			// we will support multiple Mythryl tasks
				        			// per Pthread.
    // Signal related fields:
    //
    Bool	executing_mythryl_code;				// TRUE while executing Mythryl code.
    Bool	posix_signal_pending;				// Is there a posix signal awaiting handling?
    Bool	mythryl_handler_for_posix_signal_is_running;	// Is a Mythryl signal handler active? 
    //
    Signals_Seen_And_Done_Counts
	all_posix_signals;					// Summary count for all system signals.
    //
    int		next_posix_signal_id;				// ID (e.g., SIGALRM) and 
    int		next_posix_signal_count;			// count of next signal to handle.
    //
    Signals_Seen_And_Done_Counts
	posix_signal_counts[ MAX_POSIX_SIGNALS ];		// Per-signal counts of pending signals.
    //
    int		posix_signal_rotor;				// Ihe index in previous of the next slot to check, round-robin style.
    int		cleaning_signal_handler_state;		// State of the cleaner signal handler.

    Time*	cpu_time_at_start_of_last_cleaning;		// The cumulative CPU time at the start of the last clean -- see src/c/main/timers.c
    Time*	cumulative_cleaning_cpu_time;		// The cumulative cleaning time.

    Unt32	ccall_limit_pointer_mask;			// For raw-C-call interface.

    #ifdef MULTICORE_SUPPORT
	Pid	        pid;	       				// Our kernel thread's kernel process identifier ("pid").
	Pthread_Status  status;					// See src/c/h/runtime-multicore.h
    #endif
};

#endif		// PTHREAD_H



// COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


