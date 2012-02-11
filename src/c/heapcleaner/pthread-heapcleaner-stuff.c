// pthread-heapcleaner-stuff.c
//
// For background see the "Overview" comments in:
//
//     src/lib/std/src/pthread.api
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


void   partition_agegroup0_buffer_between_pthreads   (Pthread *pthread_table[]) {		// pthread_table is always   pthread_table__global
    // ===========================================
    //
    // Outside of this file, this fn is called (only) from
    //
    //     make_task   in   src/c/main/runtime-state.c
    //
    // Divide the agegroup0 buffer into smaller disjoint
    // buffers for use by the parallel pthreads.
    //
    // Typically at this point
    //
    //     task0->heap->agegroup0_buffer_bytesize
    //
    // will at this point have been set to
    //
    //	   DEFAULT_AGEGROUP0_BUFFER_BYTESIZE  							// DEFAULT_AGEGROUP0_BUFFER_BYTESIZE is defined at 256K in   src/c/h/runtime-configuration.h
    //     *
    //     MAX_PTHREADS										// MAX_PTHREADS is defined as something like 8 or 16    in   src/c/mythryl-config.h
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
    Task* task0 =  pthread_table[ 0 ]->task;

    int per_thread_agegroup0_buffer_bytesize
	=
	task0->heap->agegroup0_master_buffer_bytesize
        /
        MAX_PTHREADS;

    Val* start_of_agegroup0_buffer_for_next_pthread
	=
	task0->heap->agegroup0_master_buffer;

static int first_call = TRUE;
								if (first_call)	log_if( "partition_agegroup0_buffer_between_pthreads: task0->heap->agegroup0_master_buffer_bytesize x=%x", task0->heap->agegroup0_master_buffer_bytesize );
								if (first_call)	log_if( "partition_agegroup0_buffer_between_pthreads: per_thread_agegroup0_buffer_bytesize   x=%x", per_thread_agegroup0_buffer_bytesize);
								if (first_call)	log_if( "partition_agegroup0_buffer_between_pthreads: task0->heap->agegroup0_master_buffer   x=%x", task0->heap->agegroup0_master_buffer);

    for (int pthread = 0;   pthread < MAX_PTHREADS;   pthread++) {
        //
	task =  pthread_table[ pthread ]->task;
									if (first_call)	log_if ( "partition_agegroup0_buffer_between_pthreads: pthread_table[%d]->task: initial  heap_allocation_pointer x=%x, heap_allocation_limit x=%x",
													 pthread, task->heap_allocation_pointer, task->heap_allocation_limit
											       );
													// PTHREAD_LOG_IF		def in   src/c/mythryl-config.h
													// HEAP_ALLOCATION_LIMIT_SIZE	def in   src/c/h/heap.h
													// This macro basically just subtracts a MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER safety margin from the actual buffer limit.

	task->heap                            =  task0->heap;
	task->heap_allocation_buffer_bytesize =  per_thread_agegroup0_buffer_bytesize;
	task->heap_allocation_buffer          =  start_of_agegroup0_buffer_for_next_pthread;
	task->heap_allocation_pointer         =  start_of_agegroup0_buffer_for_next_pthread;
if (first_call)	log_if  ("partition_agegroup0_buffer_between_pthreads: task%d->hap now x=%x",pthread, task->heap_allocation_pointer);
if (first_call)	log_if  ("partition_agegroup0_buffer_between_pthreads: per_thread_agegroup0_buffer_bytesize x=%x",per_thread_agegroup0_buffer_bytesize );
	task->real_heap_allocation_limit      =  HEAP_ALLOCATION_LIMIT( task );
if (first_call)	log_if  ("partition_agegroup0_buffer_between_pthreads: task%d->rhal now x=%x",pthread, task->real_heap_allocation_limit);

        zero_agegroup0_overrun_tripwire_buffer( task );							// zero_agegroup0_overrun_tripwire_buffer	is from   src/c/heapcleaner/heap-debug-stuff.c
	

	#if !NEED_PTHREAD_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	    //
	    task->heap_allocation_limit = task->real_heap_allocation_limit;
	#else
	    if (poll_interval <= 0) {
		//
		task->heap_allocation_limit = task->real_heap_allocation_limit;			// Same as above.
		//
	    } else {
		//
		// In order to generate software events at (approximately)
		// the desired frequency, we (may) here artificially decrease
		// the heaplimit pointer to trigger an early heapcleaner call,
		// at which point our logic will regain control.
		//
												if (first_call)	log_if ("partition_agegroup0_buffer_between_pthreads: poll_interval d=%d", poll_interval);
		task->heap_allocation_limit
		    =
		    start_of_agegroup0_buffer_for_next_pthread
		    +
		    poll_interval * PERIODIC_EVENT_TIME_GRANULARITY_IN_NEXTCODE_INSTRUCTIONS;

		task->heap_allocation_limit
		    =
		    MIN( task->heap_allocation_limit, 
			 task->real_heap_allocation_limit
		       );

	    }
	#endif

if (first_call)	log_if  ("partition_agegroup0_buffer_between_pthreads: task%d->hap x=%x, task%d->hal x=%x, task%d->rhal x=%x, hal-hap x=%x  rhal-hap x=%x",
pthread, task->heap_allocation_pointer,
pthread, task->heap_allocation_limit,
pthread, task->real_heap_allocation_limit,
(char*)(task->     heap_allocation_limit) - (char*)(task->heap_allocation_pointer),
(char*)(task->real_heap_allocation_limit) - (char*)(task->heap_allocation_pointer)
);

	// Step over this pthread's buffer to
	// get start of next pthread's buffer:
	//
	start_of_agegroup0_buffer_for_next_pthread
	    =
	    (Val*) ( ((Punt) start_of_agegroup0_buffer_for_next_pthread)
                     +
                     per_thread_agegroup0_buffer_bytesize
                   );
    }												// for (int pthread = 0;   pthread < MAX_PTHREADS;   pthread++)
first_call = FALSE;
}												// fun partition_agegroup0_buffer_between_pthreads


static volatile int	pthreads_ready_to_clean__local = 0;					// Number of processors that are ready to clean.
static volatile Tid	heapcleaner_pthread_tid__local;						// The tid (p-thread id) of the pthread that will do the actual heapcleaning work. (The rest sit and watch.)
static volatile int	barrier_needs_to_be_initialized__local;					// Not sure if these last two need to be 'volatile', but better safe than sorry.

// This holds extra roots provided by   call_heapcleaner_with_extra_roots:
//
Val*         pth__extra_heapcleaner_roots__global[ MAX_EXTRA_HEAPCLEANER_ROOTS * MAX_PTHREADS ];

static Val** extra_heapcleaner_roots__local;


int   pth__start_heapcleaning   (Task *task) {
    //=======================
    //
    // This fn is called only from
    //
    //     src/c/heapcleaner/call-heapcleaner.c
    //
    // Here we handle the start-of-heapcleaning work
    // specific to our multicore implementation.
    // Specifically, we need to
    //
    //   o Elect one pthread to do the actual heapcleaning work.
    //
    //   o Ensure that all pthreads cease running user code before
    //     heapcleaning begins.
    //
    //   o Ensure that all pthreads resume running user code after
    //     heapcleaning completes.
    //
    //
    // In more detail:
    //
    //   o First to arrive sets pthread->mode = PRIMARY_HEAPCLEANER
    //     and will be the one which does the actual heapcleaning work.
    //
    //   o The primary heapcleaner sets pth__heapcleaner_state to
    //     HEAPCLEANER_IS_STARTING to signal the remaining
    //     RUNNING pthreads to stop using the Mythryl heap and set
    //     pthread->mode == PTHREAD_IS_SECONDARY_HEAPCLEANER
    //
    //   o The primary heapcleaner waits until pth__running_pthreads_count
    //     drops to zero, then sets pth__heapcleaner_state to
    //     HEAPCLEANER_IS_RUNNING, returns to the invoking
    //     call-heapcleaner function and does the heapcleaning work
    //     while the secondary heapcleaner pthreads wait.
    //
    //   o When the primary heapcleaner completes the heapcleaning
    //     it sets pth__heapcleaner_state to HEAPCLEANER_IS_OFF and
    //     signals the secondary heapcleaner pthreads to resume
    //     execution of user code.
// log_if("pth__start_heapcleaning: TOP.");
    Pthread* pthread = task->pthread;

    pthread_mutex_lock(   &pth__mutex  );							// 
	//
	if (pth__heapcleaner_state != HEAPCLEANER_IS_OFF) {
	    ////////////////////////////////////////////////////////////
	    // We're a secondary heapcleaner -- we'll just wait() while
	    // the primary heapcleaner pthread does all the actual work:
	    ////////////////////////////////////////////////////////////
	    pthread->mode = PTHREAD_IS_SECONDARY_HEAPCLEANER;					// Remove ourself from set of RUNNING pthreads.
	    --pth__running_pthreads_count;							// Decrement count of PTHREAD_IS_RUNNING mode pthreads.
	    pthread_cond_broadcast( &pth__condvar );						// Let other pthreads know state has changed.
	    while (pth__heapcleaner_state != HEAPCLEANER_IS_OFF) {				// Wait for heapcleaning to complete.
		pthread_cond_wait(&pth__condvar,&pth__mutex);
	    }
	    pthread->mode = PTHREAD_IS_RUNNING;							// Return to RUNNING mode from SECONDARY_HEAPCLEANER mode.
	    ++pth__running_pthreads_count;
	    pthread_cond_broadcast( &pth__condvar );						// Let other pthreads know state has changed.
	    pthread_mutex_unlock(  &pth__mutex  );
// log_if("pth__start_heapcleaning: return FALSE.");
	    return FALSE;									// Resume running user code.
	}
	/////////////////////////////////////////////////////////////
	// We're the primary heapcleaner -- we'll do the actual work:
	/////////////////////////////////////////////////////////////
	pth__heapcleaner_state = HEAPCLEANER_IS_STARTING;					// Signal all PTHREAD_IS_RUNNING pthreads to block in PTHREAD_IS_SECONDARY_HEAPCLEANER
												// mode until we set pth__heapcleaner_state back to HEAPCLEANER_IS_OFF.
	pthread->mode = PTHREAD_IS_PRIMARY_HEAPCLEANER;						// Remove ourself from the set of RUNNING pthreads.
	--pth__running_pthreads_count;
	pthread_cond_broadcast( &pth__condvar );						// Let other pthreads know state has changed.

	// Clear extra-roots buffer.  This buffer gets appended to (only)
	// in pth__start_heapcleaning_with_extra_roots (below) -- other
	// pthreads may append to it as they arrive even though we don't:
	//
        pth__extra_heapcleaner_roots__global[0] =  NULL;					// Buffer must always be terminated by a NULL entry.
        extra_heapcleaner_roots__local =  pth__extra_heapcleaner_roots__global;

	while (pth__running_pthreads_count > 0) {						// Wait until all PTHREAD_IS_RUNNING pthreads have entered PTHREAD_IS_SECONDARY_HEAPCLEANER mode.
	    pthread_cond_wait( &pth__condvar, &pth__mutex);
	}
	pth__heapcleaner_state = HEAPCLEANER_IS_RUNNING;					// Note that actual heapcleaning has commenced. This is pure documentation -- nothing tests for this state.
	pthread_cond_broadcast( &pth__condvar );						// Let other pthreads know state has changed. (They don't care, but imho it is a good habit to signal each state change.)
	//
    pthread_mutex_unlock(  &pth__mutex  );							// Not logically required, but holding a mutex for a long time is a bad habit.
// log_if("pth__start_heapcleaning: return TRUE.");
    return TRUE;										// Return and run heapcleaner code.

}							// fun pth__start_heapcleaning


#ifndef OLDXTRAROOTS

int   pth__start_heapcleaning_with_extra_roots   (Task *task, va_list ap) {
    //========================================
    //
    // This fn is called (only) from:
    //
    //     src/c/heapcleaner/call-heapcleaner.c
    //
    // As above, but we collect extra roots into pth__extra_heapcleaner_roots__global.
// log_if("pth__start_heapcleaning_with_extra_roots: TOP.");


    Pthread* pthread =  task->pthread;

    pthread_mutex_lock(   &pth__mutex  );							// 
	//
	if (pth__heapcleaner_state != HEAPCLEANER_IS_OFF) {
	    ////////////////////////////////////////////////////////////
	    // We're a secondary heapcleaner -- we'll just wait() while
	    // the primary heapcleaner pthread does all the actual work:
	    ////////////////////////////////////////////////////////////
	    {   Val* p;
		while ((p = va_arg(ap, Val*)) != NULL)  *extra_heapcleaner_roots__local++ = p;	// Append our args to the  extra-roots buffer.
		*extra_heapcleaner_roots__local = p;						// Terminate extra-roots buffer with a NULL pointer.
	    }
	    pthread->mode = PTHREAD_IS_SECONDARY_HEAPCLEANER;					// Change from RUNNING to HEAPCLEANING mode.
	    --pth__running_pthreads_count;							// Increment count of PTHREAD_IS_RUNNING mode pthreads.
	    pthread_cond_broadcast( &pth__condvar );						// Let other pthreads know state has changed.

	    while (pth__heapcleaner_state != HEAPCLEANER_IS_OFF) {				// Wait for heapcleaning to complete.
		pthread_cond_wait(&pth__condvar,&pth__mutex);
	    }
	    pthread->mode = PTHREAD_IS_RUNNING;							// Return to RUNNING mode from SECONDARY_HEAPCLEANER mode.
	    ++pth__running_pthreads_count;
	    pthread_cond_broadcast( &pth__condvar );						// Let other pthreads know state has changed.
	    pthread_mutex_unlock(  &pth__mutex  );
// log_if("pth__start_heapcleaning_with_extra_roots: return FALSE.");
	    return FALSE;									// Resume running user code.
	}
	/////////////////////////////////////////////////////////////
	// We're the primary heapcleaner -- we'll do the actual work:
	/////////////////////////////////////////////////////////////
	pth__heapcleaner_state = HEAPCLEANER_IS_STARTING;					// Signal all PTHREAD_IS_RUNNING pthreads to block in PTHREAD_IS_SECONDARY_HEAPCLEANER mode
												// until we set pth__heapcleaner_state back to HEAPCLEANER_IS_OFF.
	pthread->mode = PTHREAD_IS_PRIMARY_HEAPCLEANER;						// Remove ourself from the set of PTHREAD_IS_RUNNING pthreads.
	--pth__running_pthreads_count;
	pthread_cond_broadcast( &pth__condvar );						// Let other pthreads know state has changed.

	extra_heapcleaner_roots__local = pth__extra_heapcleaner_roots__global;			// Clear extra-roots buffer.
	{   Val* p;
	    while ((p = va_arg(ap, Val*)) != NULL)  *extra_heapcleaner_roots__local++ = p;	// Append our args to the extra-roots buffer.
	    *extra_heapcleaner_roots__local = p;						// Terminate buffer with a NULL entry.
	}

	while (pth__running_pthreads_count > 0) {						// Wait until all PTHREAD_IS_RUNNING pthreads have entered PTHREAD_IS_SECONDARY_HEAPCLEANER mode.
	    pthread_cond_wait( &pth__condvar, &pth__mutex);
	}

	pth__heapcleaner_state = HEAPCLEANER_IS_RUNNING;					// Note that actual heapcleaning has begun. This is pro forma -- no code distinguishes between this state and HEAPCLEANER_IS_STARTING.
	pthread_cond_broadcast( &pth__condvar );						// Let other pthreads know state has changed. (They don't care, but imho it is a good habit to signal each state change.)
	//
    pthread_mutex_unlock(  &pth__mutex  );

// log_if("pth__start_heapcleaning_with_extra_roots: return TRUE.");
    return TRUE;										// Return and run heapcleaner code.
}												// fun pth__start_heapcleaning_with_extra_roots

#else

int   pth__start_heapcleaning_with_extra_roots   (Task *task, Roots* extra_roots) {
    //========================================
    //
    // This fn is called (only) from:
    //
    //     src/c/heapcleaner/call-heapcleaner.c
    //
    // As above, but we collect extra roots into pth__extra_heapcleaner_roots__global.
// log_if("pth__start_heapcleaning_with_extra_roots: TOP.");


    Pthread* pthread =  task->pthread;

    pthread_mutex_lock(   &pth__mutex  );							// 
	//
	if (pth__heapcleaner_state != HEAPCLEANER_IS_OFF) {

	    ////////////////////////////////////////////////////////////
	    // We're a secondary heapcleaner -- we'll just wait() while
	    // the primary heapcleaner pthread does all the actual work:
	    ////////////////////////////////////////////////////////////

	    for (Root* x = extra_roots;  x;  x = x->next ) {
		//
		*extra_heapcleaner_roots__local++ = x->root;					// Append our args to the  extra-roots buffer.
		*extra_heapcleaner_roots__local = NULL;						// Terminate extra-roots buffer with a NULL pointer.
	    }
	    pthread->mode = PTHREAD_IS_SECONDARY_HEAPCLEANER;					// Change from RUNNING to HEAPCLEANING mode.
	    --pth__running_pthreads_count;							// Increment count of PTHREAD_IS_RUNNING mode pthreads.
	    pthread_cond_broadcast( &pth__condvar );						// Let other pthreads know state has changed.

	    while (pth__heapcleaner_state != HEAPCLEANER_IS_OFF) {				// Wait for heapcleaning to complete.
		//
		pthread_cond_wait(&pth__condvar,&pth__mutex);
	    }
	    pthread->mode = PTHREAD_IS_RUNNING;							// Return to RUNNING mode from SECONDARY_HEAPCLEANER mode.

	    ++pth__running_pthreads_count;

	    pthread_cond_broadcast( &pth__condvar );						// Let other pthreads know state has changed.

	    pthread_mutex_unlock(  &pth__mutex  );

// log_if("pth__start_heapcleaning_with_extra_roots: return FALSE.");
	    return FALSE;									// Resume running user code.
	}
	/////////////////////////////////////////////////////////////
	// We're the primary heapcleaner -- we'll do the actual work:
	/////////////////////////////////////////////////////////////
	pth__heapcleaner_state = HEAPCLEANER_IS_STARTING;					// Signal all PTHREAD_IS_RUNNING pthreads to block in PTHREAD_IS_SECONDARY_HEAPCLEANER mode
												// until we set pth__heapcleaner_state back to HEAPCLEANER_IS_OFF.
	pthread->mode = PTHREAD_IS_PRIMARY_HEAPCLEANER;						// Remove ourself from the set of PTHREAD_IS_RUNNING pthreads.
	--pth__running_pthreads_count;
	pthread_cond_broadcast( &pth__condvar );						// Let other pthreads know state has changed.

	extra_heapcleaner_roots__local = pth__extra_heapcleaner_roots__global;			// Clear extra-roots buffer.

	for (Root* x = extra_roots;  x;  x = x->next){
	    //
	    *extra_heapcleaner_roots__local++ = x->root;					// Append our args to the  extra-roots buffer.
	    *extra_heapcleaner_roots__local = NULL;						// Terminate extra-roots buffer with a NULL pointer.
	}

	while (pth__running_pthreads_count > 0) {						// Wait until all PTHREAD_IS_RUNNING pthreads have entered PTHREAD_IS_SECONDARY_HEAPCLEANER mode.
	    //
	    pthread_cond_wait( &pth__condvar, &pth__mutex);
	}

	pth__heapcleaner_state = HEAPCLEANER_IS_RUNNING;					// Note that actual heapcleaning has begun. This is pro forma -- no code distinguishes between this state and HEAPCLEANER_IS_STARTING.
	pthread_cond_broadcast( &pth__condvar );						// Let other pthreads know state has changed. (They don't care, but imho it is a good habit to signal each state change.)
	//
    pthread_mutex_unlock(  &pth__mutex  );

// log_if("pth__start_heapcleaning_with_extra_roots: return TRUE.");
    return TRUE;										// Return and run heapcleaner code.
}												// fun pth__start_heapcleaning_with_extra_roots

#endif

void    pth__finish_heapcleaning   (Task*  task)   {
    //  ========================
    //
    // This fn is called (only) from
    //
    //     src/c/heapcleaner/call-heapcleaner.c
// log_if("pth__finish_heapcleaning: TOP.");

    Pthread* pthread =  task->pthread;

    // This works, but partition_agegroup0_buffer_between_pthreads is overkill:			XXX SUCKO FIXME
    //

    partition_agegroup0_buffer_between_pthreads( pthread_table__global );

    pthread_mutex_lock(   &pth__mutex  );							// 
	pth__heapcleaner_state = HEAPCLEANER_IS_OFF;						// Clear the enter-heapcleaning-mode signal.
	pthread->mode = PTHREAD_IS_RUNNING;							// Return to RUNNING mode from PRIMARY_HEAPCLEANER mode.
	++pth__running_pthreads_count;								//
	pthread_cond_broadcast( &pth__condvar );						// Let other pthreads know state has changed.
    pthread_mutex_unlock(  &pth__mutex  );
												PTHREAD_LOG_IF ("%d finished heapcleaning\n", task->pthread->tid);
// log_if("pth__finish_heapcleaning: BOTTOM.");
}



// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.





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


