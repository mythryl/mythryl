// posix-profiling-support.c
//
// Logic to increment time-quantum count of currently executing
// Mythryl function each time SIGVTALRM goes off.
//
// These counts wille eventually be tallied by
//
//     src/lib/compiler/debugging-and-profiling/profiling/write-time-profiling-report.pkg
//
// For additional background see:
//
//     src/A.TRACE-DEBUG-PROFILE.OVERVIEW

#include "../mythryl-config.h"

#include "system-dependent-unix-stuff.h"
#include "system-dependent-signal-get-set-etc.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-globals.h"
#include "profiler-call-counts.h"


// The pointer to the heap allocated rw_vector of call counts.
// When this pointer is HEAP_VOID profiling is disabled.
//
Val   time_profiling_rw_vector__global =   HEAP_VOID;

static void   sigvtalrm_handler   ();

void   start_incrementing__time_profiling_rw_vector__once_per_SIGVTALRM   () {					// Called (only) from    src/c/lib/space-and-time-profiling/libmythryl-space-and-time-profiling.c
    //
    SET_SIGNAL_HANDLER( SIGVTALRM, sigvtalrm_handler );
}

void   stop_incrementing__time_profiling_rw_vector__once_per_SIGVTALRM   () {					// Called (only) from   src/c/lib/space-and-time-profiling/libmythryl-space-and-time-profiling.c
    //
    SET_SIGNAL_HANDLER( SIGVTALRM, (void*)SIG_DFL );								// SET_SIGNAL_HANDLER	def in   src/c/h/system-dependent-signal-get-set-etc.h
	//													// SIG_DFL		def in   <signal.h> (On linux actually /usr/include/bits/signum.h, which is included by <signal.h>)
	// The above (void*) cast suppresses a spurious								// SIGVTALRM		def in   <signal.h> (On linux actually /usr/include/bits/signum.h, which is included by <signal.h>)
	//     "assignment from incompatible pointer type"
	// compiler warning, at least on x86 Linux. -- 2011-10-08 CrT
}

static void   sigvtalrm_handler   () {
    //
    // The handler for SIGVTALRM signals.
    //
    //      "SIGVTALRM is sent when a timer expires, like SIGPROF
    //       and the more popular SIGALRM. The distinction of SIGVTALRM is
    //       that its timer counts only time spent executing the process itself;
    //       SIGPROF measures time spent by the process and by the system
    //       executing on behalf of the process, while SIGALRM measures real time.
    //       It is suggested] that SIGPROF be used with SIGVTALRM to profile the
    //       time spent by the process in user space and kernel space."
    //
    //                    -- http://en.wikipedia.org/wiki/SIGVTALRM   

    Val_Sized_Unt*	rw_vector = GET_VECTOR_DATACHUNK_AS( Val_Sized_Unt*, time_profiling_rw_vector__global );		// A vector with one slot for each Mythryl function being profiled.

    int			fn_index = TAGGED_INT_TO_C_INT( DEREF( THIS_FN_PROFILING_HOOK_REFCELL_GLOBAL ) );			// 0..N-1 index of the currently executing Mythryl function.

    ++ rw_vector[ fn_index ];												// Do the increment.
}



// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


