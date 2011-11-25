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
#include "get-multipage-ram-region-from-os.h"
#include "task.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "bigcounter.h"
#include "heap.h"
#include "runtime-globals.h"
#include "runtime-timer.h"
#include "heapcleaner-statistics.h"
#include "pthread-state.h"

int all_running_pthreads_must_enter_heapcleaning_mode = FALSE;

void   partition_agegroup0_buffer_between_pthreads   (Pthread *pthread_table[]) {	// pthread_table is always   pthread_table__global
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
    //	   DEFAULT_AGEGROUP0_BUFFER_BYTESIZE  				// DEFAULT_AGEGROUP0_BUFFER_BYTESIZE is defined at 256K in   src/c/h/runtime-configuration.h
    //     *
    //     MAX_PTHREADS							// MAX_PTHREADS is defined as something like 8 or 16    in   src/c/mythryl-config.h
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
	task0->heap->agegroup0_buffer_bytesize
        /
        MAX_PTHREADS;

    Val* start_of_agegroup0_buffer_for_next_pthread
	=
	task0->heap->agegroup0_buffer;

    for (int pthread = 0;   pthread < MAX_PTHREADS;   pthread++) {
        //
	task =  pthread_table[ pthread ]->task;
										PTHREAD_LOG_IF ( "pthread_table[%d]->task-> (heap_allocation_pointer %x/heap_allocation_limit %x) changed to ",
												 pthread, task->heap_allocation_pointer, task->heap_allocation_limit
											       );

										// HEAP_ALLOCATION_LIMIT_SIZE	def in   src/c/h/heap.h
										// This macro basically just subtracts a MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER safety margin from the actual buffer limit.

	task->heap                       =  task0->heap;
	task->heap_allocation_pointer    =  start_of_agegroup0_buffer_for_next_pthread;
	task->real_heap_allocation_limit =  HEAP_ALLOCATION_LIMIT_SIZE( start_of_agegroup0_buffer_for_next_pthread, per_thread_agegroup0_buffer_bytesize );

	#if !NEED_PTHREAD_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	    //
	    task->heap_allocation_limit = task->real_heap_allocation_limit;
	#else
	    if (poll_interval <= 0) {
		//
		task->heap_allocation_limit = task->real_heap_allocation_limit;		// Same as above.
		//
	    } else {
		//
		// In order to generate software events at (approximately)
		// the desired frequency, we (may) here artificially decrease
		// the heaplimit pointer to trigger an early heapcleaner call,
		// at which point our logic will regain control.
		//
											PTHREAD_LOG_IF ("(with poll_interval=%d) ", poll_interval);
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

											PTHREAD_LOG_IF ("%x/%x\n",task->heap_allocation_pointer, task->heap_allocation_limit);

	// Step over this pthread's buffer to
	// get start of next pthread's buffer:
	//
	start_of_agegroup0_buffer_for_next_pthread
	    =
	    (Val*) ( ((Punt) start_of_agegroup0_buffer_for_next_pthread)
                     +
                     per_thread_agegroup0_buffer_bytesize
                   );
    }										// for (int pthread = 0;   pthread < MAX_PTHREADS;   pthread++)
}										// fun partition_agegroup0_buffer_between_pthreads


static volatile int	pthreads_ready_to_clean__local = 0;			// Number of processors that are ready to clean.
static volatile Pid	heapcleaner_pthread_pid__local;				// The pid of the pthread that will do the actual heapcleaning work. (The rest sit and watch.)
static volatile int	barrier_needs_to_be_initialized__local;			// Not sure if these last two need to be 'volatile', but better safe than sorry.

// This holds extra roots provided by   call_heapcleaner_with_extra_roots:
//
Val*         pth__extra_heapcleaner_roots__global[ MAX_EXTRA_HEAPCLEANER_ROOTS * MAX_PTHREADS ];

static Val** extra_cleaner_roots__local;


int   pth__start_heapcleaning   (Task *task) {
    //======================
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
    //   o The first pthread to check in becomes the
    //     designated heapcleaner, which we remember by saving its pid
    //     in heapcleaner_pthread_pid__local.
    //
    //   o The designated heapcleaner returns to the invoking
    //     call-heapcleaner fnc and does the heapcleaning work
    //     while the other pthreads wait at a barrier.
    //
    //   o When done heapcleaning, the designated heapcleaner
    //     thread checks into the barrier, releasing the remaining
    //     pthreads to resume execution of user code.

    int	     active_pthread_count;
    Pthread* pthread = task->pthread;

    // If we're the first pthread to start heapcleaning,
    // remember that and signal the remaining pthreads
    // to join in.
    //
    PTH__MUTEX_LOCK( &pth__heapcleaner_mutex__global );						// Use mutex to avoid a race condition -- otherwise multiple pthreads might think they were the designated heapcleaner.
    //
    if (pthreads_ready_to_clean__local++ == 0) {
        //
        // We're the first pthread starting heapcleaning,
	// so we'll assume the mantle of designated-heapcleaner,
	// as well as signalling the other threads to enter
	// heapcleaning mode. 
        //
        pth__extra_heapcleaner_roots__global[0] =  NULL;					// No extra roots supplied. 

        extra_cleaner_roots__local =  pth__extra_heapcleaner_roots__global;			// Why this assignment?  This global is not otherwise referenced in this fn.

	// I'm guessing the point of this is to get the other
	// pthreads to enter heapcleaning mode pronto:			-- 2011-11-02 CrT
	//
	#if NEED_PTHREAD_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	    //
	    ASSIGN( SOFTWARE_GENERATED_PERIODIC_EVENTS_SWITCH_REFCELL__GLOBAL, HEAP_TRUE );	// This refcell appears to be read only by   need_to_call_heapcleaner   in   src/c/heapcleaner/call-heapcleaner.c
	    //											// although it is also exported to the Mythryl level -- see   src/lib/std/src/unsafe/software-generated-periodic-events.api
												PTHREAD_LOG_IF ("%d: set poll event\n", task->pthread->pid);
	#endif

	heapcleaner_pthread_pid__local =  pthread->pid;						// Assume the awesome responsilibity of being the designated heapcleaner thread.

	barrier_needs_to_be_initialized__local =  TRUE;
												PTHREAD_LOG_IF ("heapcleaner_pthread_pid__local is %d\n", heapcleaner_pthread_pid__local);
    }
    PTH__MUTEX_UNLOCK( &pth__heapcleaner_mutex__global );


    //////////////////////////////////////////////////////////
    // Whether or not we're the first pthread to enter
    // heapcleaning mode, we now wait until all the
    // other active pthreads have also entered
    // heapcleaning mode.
    //
    // Note that we cannot use a barrier wait here because
    // we do not know how many pthreads will wind up entering
    // heapcleaner mode -- one or more pthreads might be starting
    // up additional pthreads.
    {
        // Spin until all active pthreads have enetered this loop:
        //
	int n = 0;
        //
	while (pthreads_ready_to_clean__local !=  (active_pthread_count = pth__get_active_pthread_count())) {	// pth__get_active_pthread_count	def in   src/c/pthread/pthread-on-posix-threads.c
	    // 
	    // Spinwait.  This is bad;
	    // to avoid being hideously bad we avoid
	    // constantly pounding the mutex (and thus
	    // the shared memory bus) by counting to 10,000
	    // on our fingers between mutex ops:
	    //
	    if (n != 1000) {
		//
		for (int i = 10000; i --> 0; );
		//
		n++;
		//
	    } else {
		//
		n = 0;
		//
		PTHREAD_LOG_IF ("%d spinning %d <> %d <alloc=0x%x, limit=0x%x>\n", 
		    task->pthread->pid, pthreads_ready_to_clean__local, active_pthread_count, task->heap_allocation_pointer,
		    task->heap_allocation_limit);
	    }
	}

	// As soon as all active pthreads have entered the above
        // loop, they all fall out and arrive here.  The first to
	// do so needs to initialize the barrier, so that everyone
	// can wait at it:
	//
        PTH__MUTEX_LOCK( &pth__heapcleaner_mutex__global );					// Use mutex to avoid a race condition.
	    //
	    if (barrier_needs_to_be_initialized__local) {
		barrier_needs_to_be_initialized__local = FALSE;					// We're the first pthread to exit the spinloop.
		//
		pth__barrier_init( &pth__heapcleaner_barrier__global, active_pthread_count );	// Set up barrier to wait on proper number of threads.
	    }
	    //
	PTH__MUTEX_UNLOCK( &pth__heapcleaner_mutex__global );

    }

    // All Pthreads are now ready to clean.

    #if NEED_PTHREAD_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	//
	ASSIGN(  SOFTWARE_GENERATED_PERIODIC_EVENTS_SWITCH_REFCELL__GLOBAL,  HEAP_FALSE  );
	//
												PTHREAD_LOG_IF ("%d: cleared poll event\n", task->pthread->pid);
    #endif

												PTHREAD_LOG_IF ("(%d) all %d/%d procs in\n", task->pthread->pid, pthreads_ready_to_clean__local, pth__get_active_pthread_count());


    ////////////////////////////////////////////////////////////////// 
    // If we're the designated heapcleaner thread
    // we now return to caller to take up our
    // heapcleaning responsibilities:
    //
    if (pthread->pid == heapcleaner_pthread_pid__local) {
        //
												PTHREAD_LOG_IF ("Heapcleaner pthread %d returning to start heapcleaning\n",pthread->pid);
        return TRUE;										// We're the designated heapcleaner -- we return and start doing the actual heapcleaning work.
    }


    ////////////////////////////////////////////////////////////////// 
    // We're not the designated heapcleaner thread,
    // so we take a break until that thread has
    // finished heapcleaning:
    //
												PTHREAD_LOG_IF ("non-heapcleaner (thread id=%d) entering barrier (barrier threshold d=%d)\n",pthread->pid,active_pthread_count);

    {   Bool                                                               i_am_the_one;	// Set by call on next line.
        char* err = pth__barrier_wait( &pth__heapcleaner_barrier__global, &i_am_the_one );	// We're not the designated heapcleaner;  wait for the designated heapcleaner to finish heapcleaning.
	    //
	    // 'i_am_the_one' will be TRUE for one pthread
	    // waiting on barrier, FALSE for the rest;
	    // We do not take advantage of that here.
	    //
	    // 'err' will be NULL normally, non-NULL only on an error;
	    // for the moment we hope for the best. XXX SUCKO FIXME.
	if (err) die(err);
    }

												PTHREAD_LOG_IF ("Non-heapcleaner pthread %d left barrier\n", pthread->pid);

    // We return FALSE to tell caller that we're
    // not the designated heapcleaner pthread, so
    // we shouldn't do any heapcleaning work upon
    // our return:
    //
    return FALSE;
}							// fun pth__start_heapcleaning


int   pth__call_heapcleaner_with_extra_roots   (Task *task, va_list ap) {
    //=====================================
    //
    // This fn is called (only) from:
    //
    //     src/c/heapcleaner/call-heapcleaner.c
    //
    // As above, but we collect extra roots into pth__extra_heapcleaner_roots__global.

    int active_pthread_count;
    Val* p;

    Pthread* pthread =  task->pthread;

    PTH__MUTEX_LOCK( &pth__heapcleaner_mutex__global );
	//
	if (pthreads_ready_to_clean__local++ == 0) {
	    //
	    extra_cleaner_roots__local = pth__extra_heapcleaner_roots__global;

	    // Signal other pthreads to enter heapcleaning mode:
	    //
	    #if NEED_PTHREAD_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
		//
		ASSIGN( SOFTWARE_GENERATED_PERIODIC_EVENTS_SWITCH_REFCELL__GLOBAL, HEAP_TRUE);
		//	
		PTHREAD_LOG_IF ("%d: set poll event\n", pthread->pid);
	    #endif

	    // We're the first one in so we'll do the heapcleaning:
	    //
	    heapcleaner_pthread_pid__local = pthread->pid;

	    barrier_needs_to_be_initialized__local =  TRUE;

	    PTHREAD_LOG_IF ("heapcleaner_pthread_pid__local is %d\n",heapcleaner_pthread_pid__local);
	}

	while ((p = va_arg(ap, Val *)) != NULL) {
	    //
	    *extra_cleaner_roots__local++ = p;
	}
	*extra_cleaner_roots__local = p;			// NULL

    PTH__MUTEX_UNLOCK( &pth__heapcleaner_mutex__global );



    //////////////////////////////////////////////////////////
    // Whether or not we're the first pthread to enter
    // heapcleaning mode, we now wait until all the
    // other active pthreads have also entered
    // heapcleaning mode.
    //
    // Note that we cannot use a barrier wait here because
    // we do not know how many pthreads will wind up entering
    // heapcleaner mode -- one or more pthreads might be starting
    // up additional pthreads.
    {
        // Spin until all active pthreads have enetered this loop:
        //
	int n = 0;
        //
	while (pthreads_ready_to_clean__local !=  (active_pthread_count = pth__get_active_pthread_count())) {

	    // SPIN

	    if (n != 1000) {
		//
		for (int i = 10000; i --> 0; );
		//
		n++;
		//
	    } else {
		//
		n = 0;
		//
												PTHREAD_LOG_IF  ( "%d spinning %d <> %d <alloc=0x%x, limit=0x%x>\n", 
														  pthread->pid, pthreads_ready_to_clean__local, active_pthread_count, task->heap_allocation_pointer,
														  task->heap_allocation_limit
														);
	    }
	}

	// As soon as all active pthreads have entered the above
        // loop, they all fall out and arrive here.  The first to
	// do so needs to initialize the barrier, so that everyone
	// can wait at it:
	//
        PTH__MUTEX_LOCK( &pth__heapcleaner_mutex__global );					// Use mutex to avoid a race condition -- otherwise multiple pthreads might think they were the designated heapcleaner.
	    //
	    if (barrier_needs_to_be_initialized__local) {
		barrier_needs_to_be_initialized__local = FALSE;					// We're the first pthread to exit the spinloop.
		//
		pth__barrier_init( &pth__heapcleaner_barrier__global, active_pthread_count );	// Set up barrier to wait on proper number of threads.
	    }
	    //
	PTH__MUTEX_UNLOCK( &pth__heapcleaner_mutex__global );
    }

    // All Pthreads now ready to clean:
    //
    #if NEED_PTHREAD_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	//
	ASSIGN(  SOFTWARE_GENERATED_PERIODIC_EVENTS_SWITCH_REFCELL__GLOBAL,  HEAP_FALSE  );
												PTHREAD_LOG_IF ("%d: cleared poll event\n", task->pthread->pid);
    #endif

												PTHREAD_LOG_IF ("(%d) all %d/%d procs in\n", task->pthread->pid, pthreads_ready_to_clean__local, pth__get_active_pthread_count());

    if (heapcleaner_pthread_pid__local == pthread->pid) {
	//
        return TRUE;										// We're the designated heapcleaner -- we return and start doing the actual heapcleaning work.
    }

												PTHREAD_LOG_IF ("pthread %d entering barrier with active_pthread_count d=%d\n", pthread->pid, active_pthread_count);

    {   Bool                                                               i_am_the_one;	// Set by call on next line.
        char* err = pth__barrier_wait( &pth__heapcleaner_barrier__global, &i_am_the_one );	// We're not the designated heapcleaner;  wait for the designated heapcleaner to finish heapcleaning.
	    //
	    // 'i_am_the_one' will be TRUE for one pthread
	    // waiting on barrier, FALSE for the rest;
	    // We do not take advantage of that here.
	    //
	    // 'err' will be NULL normally, non-NULL only on an error;
	    // for the moment we hope for the best. XXX SUCKO FIXME.
	if (err) die(err);
    }
												PTHREAD_LOG_IF ("%d left barrier\n", pthread->pid);
    // We return FALSE to tell caller that we're
    // not the designated heapcleaner pthread, so
    // we shouldn't do any heapcleaning work upon
    // our return:
    //
    return FALSE;
}												// fun pth__call_heapcleaner_with_extra_roots



void    pth__finish_heapcleaning   (Task*  task)   {
    //  ========================
    //
    // This fn is called (only) from
    //
    //     src/c/heapcleaner/call-heapcleaner.c

    // This works, but partition_agegroup0_buffer_between_pthreads is overkill:			XXX SUCKO FIXME
    //
    partition_agegroup0_buffer_between_pthreads( pthread_table__global );

    PTH__MUTEX_LOCK( &pth__heapcleaner_mutex__global );
												PTHREAD_LOG_IF ("%d entering barrier\n", task->pthread->pid );

    {   Bool                                                               i_am_the_one;
	char* err = pth__barrier_wait( &pth__heapcleaner_barrier__global, &i_am_the_one );	// We're the designated heapcleaner;  By calling this, we release all the other pthreads to resume execution of user code.
	    //											// They should all be already waiting on this barrier, so we should never block at this point.
	    // 'i_am_the_one' will be TRUE for one pthread
	    // waiting on barrier, FALSE for the rest;
	    // We do not take advantage of that here.
	    //
	    // 'err' will be NULL normally, non-NULL only on an error;
	    // for the moment we just die(). XXX SUCKO FIXME.
	if (err) die(err);
    }

    pth__barrier_destroy( &pth__heapcleaner_barrier__global );					// "destroy" is poor nomenclature -- all it does is undo what pth__barrier_init() did -- but we follow <pthread.h>'s nomenclature here.

    pthreads_ready_to_clean__local = 0;
												PTHREAD_LOG_IF ("%d left barrier\n", task->pthread->pid);
    PTH__MUTEX_UNLOCK( &pth__heapcleaner_mutex__global );
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


