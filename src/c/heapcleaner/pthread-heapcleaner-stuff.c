// pthread-heapcleaner-stuff.c
//
// Extra routines to support heapcleaning
// in the multicore implementation.

#include "../mythryl-config.h"


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
#include "runtime-pthread.h"
#include "pthread-state.h"



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


    int poll_freq
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

	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("pthread_table[%d]->task-> (heap_allocation_pointer %x/heap_allocation_limit %x) changed to ", pthread, task->heap_allocation_pointer, task->heap_allocation_limit);
	#endif

	task->heap                       =  task0->heap;
	task->heap_allocation_pointer    =  start_of_agegroup0_buffer_for_next_pthread;
	task->real_heap_allocation_limit =  HEAP_ALLOCATION_LIMIT_SIZE( start_of_agegroup0_buffer_for_next_pthread, per_thread_agegroup0_buffer_bytesize );

	#if !NEED_PTHREAD_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	    //
	    task->heap_allocation_limit
		=
		HEAP_ALLOCATION_LIMIT_SIZE(					// HEAP_ALLOCATION_LIMIT_SIZE	def in   src/c/h/heap.h
		    //								// This macro basically just subtracts a MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER safety margin from the actual buffer limit.
		    start_of_agegroup0_buffer_for_next_pthread,
		    per_thread_agegroup0_buffer_bytesize
		);
	#else
	    if (poll_freq <= 0) {
		//
		task->heap_allocation_limit = task->real_heap_allocation_limit;
		//
	    } else {
		//
		// In order to generate software events at (approximately)
		// the desired frequency, we (may) here artificially decrease
		// the heaplimit pointer to trigger an early heapcleaner call,
		// at which point our logic will regain control.
		//
		#ifdef NEED_PTHREAD_SUPPORT_DEBUG
		    debug_say ("(with poll_freq=%d) ", poll_freq);
		#endif

		task->heap_allocation_limit
		    =
		    start_of_agegroup0_buffer_for_next_pthread
		    +
		    poll_freq * PERIODIC_EVENT_TIME_GRANULARITY_IN_NEXTCODE_INSTRUCTIONS;

		task->heap_allocation_limit
		    =
		    (task->heap_allocation_limit > task->real_heap_allocation_limit)
			? task->real_heap_allocation_limit
			: task->heap_allocation_limit;

	    }
	#endif

	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("%x/%x\n",task->heap_allocation_pointer, task->heap_allocation_limit);
	#endif

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
static int		cleaning_pthread__local;					// The cleaning pthread.

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
    //     in cleaning_pthread__local.
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
    pth__acquire_mutex( &pth__heapcleaner_mutex__global );
    //
    if (pthreads_ready_to_clean__local++ == 0) {
        //
        // We're the first pthread starting heapcleaning,
	// so we'll assume the mantle of designated-heapcleaner,
	// as well as signalling the other threads to enter
	// heapcleaning mode. 
        //
        pth__extra_heapcleaner_roots__global[0] =  NULL;

        extra_cleaner_roots__local =  pth__extra_heapcleaner_roots__global;

	// I'm guessing the point of this is to get the other
	// pthreads to enter heapcleaning mode pronto:			-- 2011-11-02 CrT
	//
	#if NEED_PTHREAD_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	    //
	    ASSIGN( SOFTWARE_GENERATED_PERIODIC_EVENTS_SWITCH_REFCELL__GLOBAL, HEAP_TRUE );	// This refcell appears to be read only by   need_to_call_heapcleaner   in   src/c/heapcleaner/call-heapcleaner.c
	    //											// although it is also exported to the Mythryl level -- see   src/lib/std/src/unsafe/software-generated-periodic-events.api
	    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
		debug_say ("%d: set poll event\n", task->pid);
	    #endif
	#endif

	cleaning_pthread__local =  pthread->pid;							// Assume the awesome responsilibity of being the designated heapcleaner thread.

	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("cleaning_pthread__local is %d\n", cleaning_pthread__local);
	#endif
    }
    pth__release_mutex( &pth__heapcleaner_mutex__global );


    //////////////////////////////////////////////////////////
    // Whether or not we're the first pthread to enter
    // heapcleaning mode, we now wait until all the
    // other active pthreads have also entered
    // heapcleaning mode.
    {

/////////////////////////////////////////////////////
// Why aren't we using a regular barrier op here...? (Probably because number of threads can dynamically increase at this point.)
/////////////////////////////////////////////////////


	int n = 0;


        // NB: Some other pthread can be
        // concurrently acquiring new kernel threads.
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
		#ifdef NEED_PTHREAD_SUPPORT_DEBUG
		    //
		    debug_say ("%d spinning %d <> %d <alloc=0x%x, limit=0x%x>\n", 
			task->pidf, pthreads_ready_to_clean__local, active_pthread_count, task->heap_allocation_pointer,
			task->heap_allocation_limit);
		#endif
	    }
	}
    }

    // All Pthreads are now ready to clean.

    #if NEED_PTHREAD_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	//
	ASSIGN(  SOFTWARE_GENERATED_PERIODIC_EVENTS_SWITCH_REFCELL__GLOBAL,  HEAP_FALSE  );
	//
	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("%d: cleared poll event\n", task->pid);
	#endif
    #endif

    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say ("(%d) all %d/%d procs in\n", task->pid, pthreads_ready_to_clean__local, pth__get_active_pthread_count());
    #endif


    ////////////////////////////////////////////////////////////////// 
    // If we're the designated heapcleaner thread
    // we now return to caller to take up our
    // heapcleaning responsibilities:
    //
    if (pthread->pid == cleaning_pthread__local) {
        //
// XXX BUGGO FIXME -- Can we be sure other threads have not already reached the barrier already?
        //
	pth__barrier_init( &pth__heapcleaner_barrier__global, active_pthread_count );		// Set up barrier to wait on proper number of threads.
	//
        return TRUE;										// We're the designated heapcleaner.
    }


    ////////////////////////////////////////////////////////////////// 
    // We're not the designated heapcleaner thread,
    // so we take a break until that thread has
    // finished heapcleaning:
    //
    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say ("%d entering barrier %d\n",pthread->pid,active_pthread_count);
    #endif

    pth__barrier_wait( &pth__heapcleaner_barrier__global );			// We're not the designated heapcleaner;  wait for the designated heapcleaner to finish heapcleaning.

    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say ("%d left barrier\n", pthread->pid);
    #endif

    // We return FALSE to tell caller that we're
    // not the designated heapcleaner, so we shouldn't
    // do any heapcleaning work upon our return:
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

    pth__acquire_mutex( &pth__heapcleaner_mutex__global );

    if (pthreads_ready_to_clean__local++ == 0) {
	//
        extra_cleaner_roots__local = pth__extra_heapcleaner_roots__global;

	// Signal other pthreads to enter heapcleaning mode:
	//
	#if NEED_PTHREAD_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	    //
	    ASSIGN( SOFTWARE_GENERATED_PERIODIC_EVENTS_SWITCH_REFCELL__GLOBAL, HEAP_TRUE);
	    //	
	    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
		debug_say ("%d: set poll event\n", pthread->pid);
	    #endif
	#endif

	// We're the first one in, we'll do the collect:
	//
	cleaning_pthread__local = pthread->pid;

	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("cleaning_pthread__local is %d\n",cleaning_pthread__local);
	#endif
    }

    while ((p = va_arg(ap, Val *)) != NULL) {
        //
	*extra_cleaner_roots__local++ = p;
    }
    *extra_cleaner_roots__local = p;			// NULL

    pth__release_mutex( &pth__heapcleaner_mutex__global );

    {
	int n = 0;

	// NB: Some other pthread can be concurrently
        // acquiring new kernel threads:
        //
	while (pthreads_ready_to_clean__local !=  (active_pthread_count = pth__get_active_pthread_count())) {

	    // SPIN

	    if (n != 1000) {
		for (int i = 10000; i --> 0; );
		n++;
	    } else {
		n = 0;
		#ifdef NEED_PTHREAD_SUPPORT_DEBUG
		    debug_say ("%d spinning %d <> %d <alloc=0x%x, limit=0x%x>\n", 
			pthread->pid, pthreads_ready_to_clean__local, pthread_count, task->heap_allocation_pointer,
			task->heap_allocation_limit);
		#endif
	    }
	}
    }

    // All Pthreads now ready to clean:
    //
    #if NEED_PTHREAD_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	//
	ASSIGN(  SOFTWARE_GENERATED_PERIODIC_EVENTS_SWITCH_REFCELL__GLOBAL,  HEAP_FALSE  );
	//
	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("%d: cleared poll event\n", task->pid);
	#endif
    #endif

    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say ("(%d) all %d/%d procs in\n", task->pthread->pid, pthreads_ready_to_clean__local, pth__get_active_pthread_count());
    #endif

    if (cleaning_pthread__local == pthread->pid) {
	//
// XXX BUGGO FIXME -- Can we be sure other threads have not already reached the barrier already?
	//
	pth__barrier_init( &pth__heapcleaner_barrier__global, active_pthread_count );		// Set up barrier to wait on proper number of threads.
	//
        return TRUE;			// We're the designated heapcleaner.
    }

    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say ("%d entering barrier %d\n", pthread->pid, pthread_count);
    #endif

    pth__barrier_wait( &pth__heapcleaner_barrier__global );			// We're not the designated heapcleaner;  wait for the designated heapcleaner to finish heapcleaning.

    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say ("%d left barrier\n", pthread->pid);
    #endif

    return 0;
}												// fun pth__call_heapcleaner_with_extra_roots



void    pth__finish_heapcleaning   (Task*  task)   {
    //  ========================
    //
    // This fn is called only from
    //
    //     src/c/heapcleaner/call-heapcleaner.c
    //
    // 

    // This works, but partition_agegroup0_buffer_between_pthreads is overkill:		XXX BUGGO FIXME
    //
    partition_agegroup0_buffer_between_pthreads( pthread_table__global );

    pth__acquire_mutex( &pth__heapcleaner_mutex__global );

    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say ("%d entering barrier\n", task->pthread->pid );
    #endif

    pth__barrier_wait( &pth__heapcleaner_barrier__global );					// We're the designated heapcleaner;  By calling this, we release all the other pthreads to resume execution of user code.
												// They should all be already waiting on this barrier, so we should never block at this point.
    pthreads_ready_to_clean__local = 0;

    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say ("%d left barrier\n", task->pthread->pid);
    #endif

    pth__release_mutex( &pth__heapcleaner_mutex__global );
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


