// libmythryl-space-and-time-profiling.c

// This file defines the "profile" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  get_sigvtalrm_interval_in_microseconds:  Void -> Int
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "profile", fun_name => "get_sigvtalrm_interval_in_microseconds" };
// 
// or such -- see src/lib/std/src/nj/runtime-profiling-control.pkg
//
// For background see:
//
//     src/A.TRACE-DEBUG-PROFILE.OVERVIEW


#include "../../mythryl-config.h"

#ifdef OPSYS_UNIX
#  include "system-dependent-unix-stuff.h"
#if HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif
#endif


#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-globals.h"

#include "make-strings-and-vectors-etc.h"
#include "mythryl-callable-c-libraries.h"

#include "profiler-call-counts.h"
#include "raise-error.h"




extern void   start_incrementing__time_profiling_rw_vector__once_per_SIGVTALRM    (void);			// From   src/c/machine-dependent/posix-profiling-support.c
extern void    stop_incrementing__time_profiling_rw_vector__once_per_SIGVTALRM   (void);			// From   src/c/machine-dependent/posix-profiling-support.c



// One of the library bindings exported via
//     src/c/lib/space-and-time-profiling/cfun-list.h
// and thence
//     src/c/lib/space-and-time-profiling/libmythryl-space-and-time-profiling.c



static Val   set_time_profiling_rw_vector   (Task* task,  Val arg)   {
    //       ============================
    //
    // Mythryl type:   Null_Or(Rw_Vector(Unt)) -> Void
    //
    // This dis/ables handling of SIGVTALRM signals by the process,
    // vs  set__time_profiling_is_running__to() below which
    // dis/ables sending of SIGVTALRM signals to the process,
    //
    // Set the profile array reference;
    // NULL means that there is no array.
    //
    // This function is bound as    set_time_profiling_rw_vector'   in:
    //
    //     src/lib/std/src/nj/runtime-profiling-control.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("set_time_profiling_rw_vector");

#ifdef OPSYS_UNIX

    Bool  enabled =   (time_profiling_rw_vector__global != HEAP_VOID);
    int	    i;

    if (arg != OPTION_NULL) {

	time_profiling_rw_vector__global =   OPTION_GET( arg );

	if (!enabled) {
	    //
	    c_roots__global[c_roots_count__global++] = &time_profiling_rw_vector__global;		// Add   time_profiling_rw_vector__global   to the C roots.
            //
	    start_incrementing__time_profiling_rw_vector__once_per_SIGVTALRM ();		// Enable SIGVTALRM profiling signals   via   src/c/machine-dependent/posix-profiling-support.c
	}

    } else if (enabled) {

        // Remove   time_profiling_rw_vector__global   from the C roots:
        //
	for (i = 0;  i < c_roots_count__global;  i++) {
	    //
	    if (c_roots__global[i] == &time_profiling_rw_vector__global) {
		c_roots__global[i] = c_roots__global[ --c_roots_count__global ];
		break;
	    }
	}

        // Disable profiling signals:
        //
	stop_incrementing__time_profiling_rw_vector__once_per_SIGVTALRM ();			// Disable SIGVTALRM profiling signals   via   src/c/machine-dependent/posix-profiling-support.c
	time_profiling_rw_vector__global =  HEAP_VOID;
    }

    return HEAP_VOID;
#else
    return RAISE_ERROR__MAY_HEAPCLEAN(task, "time profiling not supported", NULL);
#endif

}



static Val   set__time_profiling_is_running__to   (Task* task,  Val arg)   {
    //       ==================================
    //
    // Mythryl type:   Bool -> Void
    //
    // This dis/ables sending of SIGVTALRM signals to the process,
    // vs  set_time_profiling_rw_vector() above which
    // dis/ables handling of SIGVTALRM signals by the process.
    //
    // This fn gets bound to   set__time_profiling_is_running__to   in:
    //
    //     src/lib/std/src/nj/runtime-profiling-control.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("set__time_profiling_is_running__to");

    #ifndef HAS_SETITIMER
	//
	return RAISE_ERROR__MAY_HEAPCLEAN(task, "time profiling not supported", NULL);
	//
    #else
	//     "The system provides each process with three interval timers,
	//      each decrementing in a distinct time domain. When any timer
	//      expires, a signal is sent to the process, and the timer
	//      (potentially) restarts.
	//
	//          ITIMER_REAL	   Decrements in real time, and delivers SIGALRM upon expiration.
	//          ITIMER_VIRTUAL Decrements only when the process is executing, and delivers SIGVTALRM upon expiration.
	//          ITIMER_PROF	   Decrements both when the process executes and when the system is executing on behalf of the process.
	//                         Coupled with ITIMER_VIRTUAL, this timer is usually used to profile the time spent by the application
	//			   in user and kernel space. SIGPROF is delivered upon expiration.
	//
	//            -- http://linux.about.com/library/cmd/blcmdl2_setitimer.htm
	//
	struct itimerval   new_itv;

	if (arg == HEAP_FALSE) {
	    //
	    new_itv.it_interval.tv_sec	=
	    new_itv.it_value.tv_sec	=
	    new_itv.it_interval.tv_usec	=
	    new_itv.it_value.tv_usec	= 0;

	} else if (time_profiling_rw_vector__global == HEAP_VOID) {
	    //
	    return RAISE_ERROR__MAY_HEAPCLEAN(task, "no time_profiling_rw_vector set", NULL);

	} else {
	    //
	    new_itv.it_interval.tv_sec	=
	        new_itv.it_value.tv_sec	= 0;

	    new_itv.it_interval.tv_usec	=
	       new_itv.it_value.tv_usec	= MICROSECONDS_PER_SIGVTALRM;		// From   src/c/h/profiler-call-counts.h
	}

	int status = setitimer (ITIMER_VIRTUAL, &new_itv, NULL);

	return  RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);

    #endif
}



static Val   get_sigvtalrm_interval_in_microseconds   (Task* task,  Val arg)   {
    //       ======================================
    //
    // Mythryl type:   Void -> int
    //
    // Return the profile timer quantim in microseconds.
    //
    // This fn gets bound as   get_sigvtalrm_interval_in_microseconds   in:
    //
    //     src/lib/std/src/nj/runtime-profiling-control.pkg
    //
    // This fn is currently only called in:
    //
    //     src/lib/compiler/debugging-and-profiling/profiling/write-time-profiling-report.pkg	
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("get_sigvtalrm_interval_in_microseconds");

    return TAGGED_INT_FROM_C_INT( MICROSECONDS_PER_SIGVTALRM );			// From   src/c/h/profiler-call-counts.h
}



static Mythryl_Name_With_C_Function CFunTable[] = {
    //
    { "set__time_profiling_is_running__to",            "set_time_profiling_is_running_to",		set__time_profiling_is_running__to,		"Bool -> Void"				},
    { "get_sigvtalrm_interval_in_microseconds",		"get_sigvtalrm_interval_in_microseconds",	get_sigvtalrm_interval_in_microseconds,		"Void -> Int"  				},
    { "set_time_profiling_rw_vector",			"set_time_profiling_rw_vector",			set_time_profiling_rw_vector,			"Null_Or(Rw_Vector(Unt)) -> Void"	},
    CFUNC_NULL_BIND
};


// The space-and-time code-execution profiling library:
//
// Our record                Libmythryl_Time_And_Space_Profiling
// gets compiled into        src/c/lib/mythryl-callable-c-libraries-list.h
// and thus ultimately       mythryl_callable_c_libraries__local []
// in                        src/c/lib/mythryl-callable-c-libraries.c
//
Mythryl_Callable_C_Library	    Libmythryl_Time_And_Space_Profiling = {						// Mythryl_Callable_C_Library		def in    src/c/h/mythryl-callable-c-libraries.h
    //                              ==================
    "profile",			// Library name.
    "1.0",			// Library version.
    "December 15, 1994",	// Library creation date.
    NULL,
    CFunTable			// Library functions.
};



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

