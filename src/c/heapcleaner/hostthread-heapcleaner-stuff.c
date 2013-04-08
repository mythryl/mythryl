// hostthread-heapcleaner-stuff.c
//
// For background see the "Overview" comments in:
//
//     src/lib/std/src/hostthread.api
//
// Extra routines to support heapcleaning
// in the multicore implementation.

#include "../mythryl-config.h"

#include <stdio.h>

#if HAVE_SYS_TIME_H
    #include <sys/time.h>
#endif

#include <stdarg.h>
#include "runtime-base.h"
#include "runtime-configuration.h"
#include "get-quire-from-os.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "bigcounter.h"
#include "heap.h"
#include "runtime-globals.h"
#include "runtime-timer.h"
#include "heapcleaner-statistics.h"


void   partition_agegroup0_buffer_between_hostthreads   (Hostthread *hostthread_table[]) {		// hostthread_table is always   hostthread_table__global
    // ===========================================
    //
    // Outside of this file, this fn is called (only) from
    //
    //     make_task   in   src/c/main/runtime-state.c
    //
    // Divide the agegroup0 buffer into smaller disjoint
    // buffers for use by the parallel hostthreads.
    //
    // Typically at this point
    //
    //     task0->heap->agegroup0_buffer_bytesize
    //
    // will at this point have been set to
    //
    //	   DEFAULT_AGEGROUP0_BUFFER_BYTESIZE  								// DEFAULT_AGEGROUP0_BUFFER_BYTESIZE is defined at 256K in   src/c/h/runtime-configuration.h
    //     *
    //     MAX_HOSTTHREADS										// MAX_HOSTTHREADS is defined as something like 8 or 16    in   src/c/mythryl-config.h
    //
    // by the logic in
    //
    //     src/c/heapcleaner/heapcleaner-initialization.c
    //     


    int poll_interval
	=
	TAGGED_INT_TO_C_INT(
	    DEREF(
		SOFTWARE_GENERATED_PERIODIC_EVENT_INTERVAL_REFCELL__GLOBAL
	    )
	);

    Task* task;
    Task* task0 =  hostthread_table[ 0 ]->task;

    int per_hostthread_agegroup0_buffer_bytesize
	=
	task0->heap->agegroup0_master_buffer_bytesize
        /
        MAX_HOSTTHREADS;

    Val* start_of_agegroup0_buffer_for_next_hostthread
	=
	task0->heap->agegroup0_master_buffer;

 static int first_call = TRUE;
 if (first_call)
 log_if( "partition_agegroup0_buffer_between_hostthreads: task0->heap->agegroup0_master_buffer_bytesize x=%x",
 task0->heap->agegroup0_master_buffer_bytesize );
 if (first_call)
 log_if( "partition_agegroup0_buffer_between_hostthreads: per_hostthread_agegroup0_buffer_bytesize   x=%x",
 per_hostthread_agegroup0_buffer_bytesize);
 if (first_call)
 log_if( "partition_agegroup0_buffer_between_hostthreads: task0->heap->agegroup0_master_buffer   x=%x",
 task0->heap->agegroup0_master_buffer);

    for (int hostthread = 0;   hostthread < MAX_HOSTTHREADS;   hostthread++) {
        //
	task =  hostthread_table[ hostthread ]->task;

 if (first_call)
 log_if ( "partition_agegroup0_buffer_between_hostthreads: hostthread_table[%d]->task: initial  heap_allocation_pointer x=%x, heap_allocation_limit x=%x",
 hostthread, task->heap_allocation_pointer, task->heap_allocation_limit
 );
													// HOSTTHREAD_LOG_IF		def in   src/c/mythryl-config.h
													// HEAP_ALLOCATION_LIMIT_SIZE	def in   src/c/h/heap.h
													// This macro basically just subtracts a MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER safety margin from the actual buffer limit.

	task->heap                            =  task0->heap;
	task->heap_allocation_buffer_bytesize =  per_hostthread_agegroup0_buffer_bytesize;
	task->heap_allocation_buffer          =  start_of_agegroup0_buffer_for_next_hostthread;
	task->heap_allocation_pointer         =  start_of_agegroup0_buffer_for_next_hostthread;

													if (first_call)	log_if  ("partition_agegroup0_buffer_between_hostthreads: task%d->hap now x=%x",
													hostthread, task->heap_allocation_pointer);
													if (first_call)
													log_if("partition_agegroup0_buffer_between_hostthreads: per_hostthread_agegroup0_buffer_bytesize x=%x",
													per_hostthread_agegroup0_buffer_bytesize );

	task->real_heap_allocation_limit      =  HEAP_ALLOCATION_LIMIT( task );				// HEAP_ALLOCATION_LIMIT	is from   src/c/h/heap.h

													if (first_call)
													log_if("partition_agegroup0_buffer_between_hostthreads: task%d->rhal now x=%x",
													hostthread, task->real_heap_allocation_limit);

        initialize_agegroup0_overrun_tripwire_buffer( task );						// initialize_agegroup0_overrun_tripwire_buffer	is from   src/c/heapcleaner/heap-debug-stuff.c
	

	#if !NEED_HOSTTHREAD_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	    //
	    task->heap_allocation_limit = task->real_heap_allocation_limit;
	#else
	    if (poll_interval <= 0) {
		//
		task->heap_allocation_limit = task->real_heap_allocation_limit;				// Same as above.
		//
	    } else {
		//
		// In order to generate software events at (approximately)
		// the desired frequency, we here artificially decrease
		// the heaplimit pointer to trigger an early heapcleaner call,
		// at which point our logic will regain control.
		//
													if (first_call)	log_if ("partition_agegroup0_buffer_between_hostthreads: poll_interval d=%d", poll_interval);
		task->heap_allocation_limit
		    =
		    start_of_agegroup0_buffer_for_next_hostthread
		    +
		    poll_interval * PERIODIC_EVENT_TIME_GRANULARITY_IN_NEXTCODE_INSTRUCTIONS;

		task->heap_allocation_limit
		    =
		    MIN( task->heap_allocation_limit, 
			 task->real_heap_allocation_limit
		       );

	    }
	#endif

													if (first_call)	log_if  ("partition_agegroup0_buffer_between_hostthreads: task%d->hap x=%x, task%d->hal x=%x, task%d->rhal x=%x, hal-hap x=%x  rhal-hap x=%x",
													hostthread, task->heap_allocation_pointer,
													hostthread, task->heap_allocation_limit,
													hostthread, task->real_heap_allocation_limit,
													(char*)(task->     heap_allocation_limit) - (char*)(task->heap_allocation_pointer),
													(char*)(task->real_heap_allocation_limit) - (char*)(task->heap_allocation_pointer)
													);

	// Step over this hostthread's buffer to
	// get start of next hostthread's buffer:
	//
	start_of_agegroup0_buffer_for_next_hostthread
	    =
	    (Val*) ( ((Vunt) start_of_agegroup0_buffer_for_next_hostthread)
                     +
                     per_hostthread_agegroup0_buffer_bytesize
                   );
    }													// for (int hostthread = 0;   hostthread < MAX_HOSTTHREADS;   hostthread++)
													first_call = FALSE;
}													// fun partition_agegroup0_buffer_between_hostthreads


 static volatile int	hostthreads_ready_to_clean__local = 0;						// Number of processors that are ready to clean.
 static volatile Ptid	heapcleaner_hostthread_tid__local;						// The tid (p-thread id) of the hostthread that will do the actual heapcleaning work. (The rest sit and watch.)
 static volatile int	barrier_needs_to_be_initialized__local;						// Not sure if these last two need to be 'volatile', but better safe than sorry.

// This holds extra roots provided by   call_heapcleaner_with_extra_roots:
//
#define MAX_EXTRA_HEAPCLEANER_ROOTS	(MAX_EXTRA_HEAPCLEANER_ROOTS_PER_HOSTTHREAD * MAX_HOSTTHREADS)
//
Val* pth__extra_heapcleaner_roots__global[ MAX_EXTRA_HEAPCLEANER_ROOTS ];
//
static Val** extra_heapcleaner_roots__local;

//
void    pth__validate_running_hostthreads_count   (void)   {
    //  ====================================
    //
    // Check that		    pth__running_hostthreads_count__global
    // is correct by looping over   hostthread_table__global[]					// hostthread_table__global	def in   src/c/main/runtime-state.c
    // and counting.
    // 
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !! CALLER MUST BE HOLDING pth__mutex !!
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // 
    // (Otherwise pth__running_hostthreads_count__global and/or
    // the 'mode' fields might change in the middle
    // of the below loop, invalidating the computation.)	 

    int running_hostthreads = 0;

    for (int i = 0;  i < MAX_HOSTTHREADS;  ++i) {
	//
	if (hostthread_table__global[i]->mode == HOSTTHREAD_IS_RUNNING) {
	    //
	    ++ running_hostthreads;
	}
    }

    if (running_hostthreads != pth__running_hostthreads_count__global) {
	//
	die("src/c/heapcleaner/hostthread-heapcleaner-stuff.c: validate_running_hostthreads_count: pth__running_hostthreads_count__global d=%d but running_hostthreads d=%d!",
	    pth__running_hostthreads_count__global,
	    running_hostthreads
	);
    }
}

//
int   pth__start_heapcleaning   (Task *task) {
    //=======================
    //
    // This fn is called only from   call_heapcleaner   in
    //
    //     src/c/heapcleaner/call-heapcleaner.c
    //
    // Here we handle the start-of-heapcleaning work
    // specific to our multicore implementation.
    // Specifically, we need to
    //
    //   o Elect one hostthread to do the actual heapcleaning work.
    //
    //   o Ensure that all hostthreads cease running user code before
    //     heapcleaning begins.
    //
    //   o Ensure that all hostthreads resume running user code after
    //     heapcleaning completes.
    //
    //
    // In more detail:
    //
    //   o First to arrive sets hostthread->mode = PRIMARY_HEAPCLEANER
    //     and will be the one which does the actual heapcleaning work.
    //
    //   o The primary heapcleaner sets pth__heapcleaner_state__global to
    //     HEAPCLEANER_IS_STARTING to signal the remaining
    //     RUNNING hostthreads to stop using the Mythryl heap and set
    //     hostthread->mode == HOSTTHREAD_IS_SECONDARY_HEAPCLEANER
    //
    //   o The primary heapcleaner waits until pth__running_hostthreads_count__global
    //     drops to zero, then sets pth__heapcleaner_state__global to
    //     HEAPCLEANER_IS_RUNNING, returns to the invoking
    //     call-heapcleaner function and does the heapcleaning work
    //     while the secondary heapcleaner hostthreads wait.
    //
    //   o When the primary heapcleaner completes the heapcleaning
    //     it sets pth__heapcleaner_state__global to HEAPCLEANER_IS_OFF and
    //     signals the secondary heapcleaner hostthreads to resume
    //     execution of user code.
// log_if("pth__start_heapcleaning: TOP.");
    Hostthread* hostthread = task->hostthread;

    pthread_mutex_lock(   &pth__mutex  );							// 
	//
	if (pth__heapcleaner_state__global != HEAPCLEANER_IS_OFF) {
	    ////////////////////////////////////////////////////////////
	    // We're a secondary heapcleaner -- we'll just wait() while
	    // the primary heapcleaner hostthread does all the actual work:
	    ////////////////////////////////////////////////////////////
	    hostthread->mode = HOSTTHREAD_IS_SECONDARY_HEAPCLEANER;				// Remove ourself from set of RUNNING hostthreads.
	    --pth__running_hostthreads_count__global;						// Decrement count of HOSTTHREAD_IS_RUNNING mode hostthreads.
	    pthread_cond_broadcast( &pth__condvar );						// Let other hostthreads know state has changed.
	    while (pth__heapcleaner_state__global != HEAPCLEANER_IS_OFF) {			// Wait for heapcleaning to complete.
		pthread_cond_wait(&pth__condvar,&pth__mutex);					// (pth__mutex is released while waiting and returned to us before waking.)
	    }
	    hostthread->mode = HOSTTHREAD_IS_RUNNING;						// Return to RUNNING mode from SECONDARY_HEAPCLEANER mode.
	    ++pth__running_hostthreads_count__global;
	    pthread_cond_broadcast( &pth__condvar );						// Let other hostthreads know state has changed.
	    pthread_mutex_unlock(  &pth__mutex  );
// log_if("pth__start_heapcleaning: return FALSE.");
	    return FALSE;									// Resume running user code.
	}
	/////////////////////////////////////////////////////////////
	// We're the primary heapcleaner -- we'll do the actual work:
	/////////////////////////////////////////////////////////////
												pth__validate_running_hostthreads_count();

	pth__heapcleaner_state__global = HEAPCLEANER_IS_STARTING;				// Signal all HOSTTHREAD_IS_RUNNING hostthreads to block in HOSTTHREAD_IS_SECONDARY_HEAPCLEANER
												// mode until we set pth__heapcleaner_state__global back to HEAPCLEANER_IS_OFF.
	hostthread->mode = HOSTTHREAD_IS_PRIMARY_HEAPCLEANER;					// Remove ourself from the set of RUNNING hostthreads.
	--pth__running_hostthreads_count__global;
	pthread_cond_broadcast( &pth__condvar );						// Let other hostthreads know state has changed.

	// Clear extra-roots buffer.  This buffer gets appended to (only)
	// in pth__start_heapcleaning_with_extra_roots (below) -- other
	// hostthreads may append to it as they arrive even though we don't:
	//
        pth__extra_heapcleaner_roots__global[0] =  NULL;					// Buffer must always be terminated by a NULL entry.
        extra_heapcleaner_roots__local =  pth__extra_heapcleaner_roots__global;

	while (pth__running_hostthreads_count__global > 0) {					// Wait until all HOSTTHREAD_IS_RUNNING hostthreads have entered HOSTTHREAD_IS_SECONDARY_HEAPCLEANER mode.
	    pthread_cond_wait( &pth__condvar, &pth__mutex);
	}
	pth__heapcleaner_state__global = HEAPCLEANER_IS_RUNNING;				// Note that actual heapcleaning has commenced. This is pure documentation -- nothing tests for this state.
	pthread_cond_broadcast( &pth__condvar );						// Let other hostthreads know state has changed. (They don't care, but imho it is a good habit to signal each state change.)
	//
    pthread_mutex_unlock(  &pth__mutex  );							// Not logically required, but holding a mutex for a long time is a bad habit.
// log_if("pth__start_heapcleaning: return TRUE.");
    return TRUE;										// Return and run heapcleaner code.

}							// fun pth__start_heapcleaning

//
int   pth__start_heapcleaning_with_extra_roots   (Task *task,  Roots* extra_roots) {
    //========================================
    //
    // This fn is called (only) from   call_heapcleaner_with_extra_roots   in
    //
    //     src/c/heapcleaner/call-heapcleaner.c
    //
    // As above, but we collect extra roots into pth__extra_heapcleaner_roots__global.
// log_if("pth__start_heapcleaning_with_extra_roots: TOP.");


    Hostthread* hostthread =  task->hostthread;

    pthread_mutex_lock(   &pth__mutex  );							// 
	//
	if (pth__heapcleaner_state__global != HEAPCLEANER_IS_OFF) {

	    ////////////////////////////////////////////////////////////
	    // We're a secondary heapcleaner -- we'll just wait() while
	    // the primary heapcleaner hostthread does all the actual work:
	    ////////////////////////////////////////////////////////////

	    for (Roots* x = extra_roots;  x;  x = x->next ) {
		//
		*extra_heapcleaner_roots__local++ =  x->root;					// Append our args to the  extra-roots buffer.
	    }
	    *extra_heapcleaner_roots__local = NULL;						// Terminate extra-roots buffer with a NULL pointer.

												if (extra_heapcleaner_roots__local >=  &pth__extra_heapcleaner_roots__global[ MAX_EXTRA_HEAPCLEANER_ROOTS ]) {
												    die("src/c/heapcleaner/hostthread-heapcleaner-stuff.c: pth__extra_heapcleaner_roots__global[] overflow.");
												} 

	    hostthread->mode = HOSTTHREAD_IS_SECONDARY_HEAPCLEANER;				// Change from RUNNING to HEAPCLEANING mode.
	    --pth__running_hostthreads_count__global;						// Increment count of HOSTTHREAD_IS_RUNNING mode hostthreads.

	    pthread_cond_broadcast( &pth__condvar );						// Let other hostthreads know state has changed.

	    while (pth__heapcleaner_state__global != HEAPCLEANER_IS_OFF) {			// Wait for heapcleaning to complete.
		//
		pthread_cond_wait(&pth__condvar,&pth__mutex);
	    }
	    hostthread->mode = HOSTTHREAD_IS_RUNNING;						// Return to RUNNING mode from SECONDARY_HEAPCLEANER mode.

	    ++pth__running_hostthreads_count__global;

	    pthread_cond_broadcast( &pth__condvar );						// Let other hostthreads know state has changed.

	    pthread_mutex_unlock(  &pth__mutex  );

// log_if("pth__start_heapcleaning_with_extra_roots: return FALSE.");
	    return FALSE;									// Resume running user code.
	}
	/////////////////////////////////////////////////////////////
	// We're the primary heapcleaner -- we'll do the actual work:
	/////////////////////////////////////////////////////////////
												pth__validate_running_hostthreads_count();

	pth__heapcleaner_state__global = HEAPCLEANER_IS_STARTING;				// Signal all HOSTTHREAD_IS_RUNNING hostthreads to block in HOSTTHREAD_IS_SECONDARY_HEAPCLEANER mode
												// until we set pth__heapcleaner_state__global back to HEAPCLEANER_IS_OFF.
	hostthread->mode = HOSTTHREAD_IS_PRIMARY_HEAPCLEANER;					// Remove ourself from the set of HOSTTHREAD_IS_RUNNING hostthreads.
	--pth__running_hostthreads_count__global;
	pthread_cond_broadcast( &pth__condvar );						// Let other hostthreads know state has changed.

	extra_heapcleaner_roots__local = pth__extra_heapcleaner_roots__global;			// Clear extra-roots buffer.

	for (Roots* x = extra_roots;  x;  x = x->next){
	    //
	    *extra_heapcleaner_roots__local++ =  x->root;					// Append our args to the  extra-roots buffer.
	}
	*extra_heapcleaner_roots__local = NULL;							// Terminate extra-roots buffer with a NULL pointer.

												if (extra_heapcleaner_roots__local >=  &pth__extra_heapcleaner_roots__global[ MAX_EXTRA_HEAPCLEANER_ROOTS ]) {
												    die("src/c/heapcleaner/hostthread-heapcleaner-stuff.c: pth__extra_heapcleaner_roots__global[] overflow.");
												} 

	while (pth__running_hostthreads_count__global > 0) {					// Wait until all HOSTTHREAD_IS_RUNNING hostthreads have entered HOSTTHREAD_IS_SECONDARY_HEAPCLEANER mode.
	    //
	    pthread_cond_wait( &pth__condvar, &pth__mutex);
	}

	pth__heapcleaner_state__global = HEAPCLEANER_IS_RUNNING;				// Note that actual heapcleaning has begun. This is pro forma -- no code distinguishes between this state and HEAPCLEANER_IS_STARTING.
	pthread_cond_broadcast( &pth__condvar );						// Let other hostthreads know state has changed. (They don't care, but imho it is a good habit to signal each state change.)
	//
    pthread_mutex_unlock(  &pth__mutex  );

// log_if("pth__start_heapcleaning_with_extra_roots: return TRUE.");
    return TRUE;										// Return and run heapcleaner code.
}												// fun pth__start_heapcleaning_with_extra_roots

//
void    pth__finish_heapcleaning   (Task*  task)   {
    //  ========================
    //
    // This fn is called (only) from
    //
    //     src/c/heapcleaner/call-heapcleaner.c
// log_if("pth__finish_heapcleaning: TOP.");

    Hostthread* hostthread =  task->hostthread;

    // This works, but partition_agegroup0_buffer_between_hostthreads is overkill:			XXX SUCKO FIXME
    //

    partition_agegroup0_buffer_between_hostthreads( hostthread_table__global );

    pthread_mutex_lock(   &pth__mutex  );							// 
	pth__heapcleaner_state__global = HEAPCLEANER_IS_OFF;					// Clear the enter-heapcleaning-mode signal.
	hostthread->mode = HOSTTHREAD_IS_RUNNING;						// Return to RUNNING mode from PRIMARY_HEAPCLEANER mode.
	++pth__running_hostthreads_count__global;						//
	pthread_cond_broadcast( &pth__condvar );						// Let other hostthreads know state has changed.
    pthread_mutex_unlock(  &pth__mutex  );
												HOSTTHREAD_LOG_IF ("%lx finished heapcleaning\n", (unsigned long int) task->hostthread->ptid);
// log_if("pth__finish_heapcleaning: BOTTOM.");
}




// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.





/*
##########################################################################
#   The following is support for outline-minor-mode in emacs.		 #
#  ^C @ ^T hides all Text. (Leaves all headings.)			 #
#  ^C @ ^A shows All of file.						 #
#  ^C @ ^Q Quickfolds entire file. (Leaves only top-level headings.)	 #
#  ^C @ ^I shows Immediate children of node.				 #
#  ^C @ ^S Shows all of a node.						 #
#  ^C @ ^D hiDes all of a node.						 #
#  ^HFoutline-mode gives more details.					 #
#  (Or do ^HI and read emacs:outline mode.)				 #
#									 #
# Local variables:							 #
# mode: outline-minor							 #
# outline-regexp: "[A-Za-z]"			 		 	 #
# End:									 #
##########################################################################
*/


