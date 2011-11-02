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



// Outside of this file, this fn is called (only) from
//
//     make_task   in   src/c/main/runtime-state.c
//
void   partition_agegroup0_buffer_between_pthreads   (Pthread *pthread_table[]) {	// pthread_table is always   pthread_table_global
    // ==========================
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
		SOFTWARE_GENERATED_PERIODIC_EVENT_INTERVAL_REFCELL_GLOBAL
	    )
	);

    Task* task;
    Task* task0 =  pthread_table[ 0 ]->task;

    int per_thread_agegroup0_buffer_bytesize
	=
	task0->heap->agegroup0_buffer_bytesize
        /
        MAX_PTHREADS;

    Val* agregroup0_buffer =  task0->heap->agegroup0_buffer;

    for (int pthread = 0;   pthread < MAX_PTHREADS;   pthread++) {
        //
	task = pthread_table[pthread]->task;

	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("pthread_table[%d]->task-> (heap_allocation_pointer %x/heap_allocation_limit %x) changed to ", pthread, task->heap_allocation_pointer, task->heap_allocation_limit);
	#endif

	task->heap            =  task0->heap;
	task->heap_allocation_pointer     =  agregroup0_buffer;
	task->real_heap_allocation_limit =  HEAP_ALLOCATION_LIMIT_SIZE( agregroup0_buffer, per_thread_agegroup0_buffer_bytesize );

	#ifdef MULTICORE_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	    if (poll_freq > 0) {

		#ifdef NEED_PTHREAD_SUPPORT_DEBUG
		debug_say ("(with poll_freq=%d) ", poll_freq);
		#endif

		task->heap_allocation_limit =  agregroup0_buffer + poll_freq * PERIODIC_EVENT_TIME_GRANULARITY_IN_NEXTCODE_INSTRUCTIONS;

		task->heap_allocation_limit
		    =
		    (task->heap_allocation_limit > task->real_heap_allocation_limit)
			? task->real_heap_allocation_limit
			: task->heap_allocation_limit;

	    } else {
		task->heap_allocation_limit = task->real_heap_allocation_limit;
	    }
	#else
	    task->heap_allocation_limit = HEAP_ALLOCATION_LIMIT_SIZE( agregroup0_buffer, per_thread_agegroup0_buffer_bytesize );
	#endif

	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("%x/%x\n",task->heap_allocation_pointer, task->heap_allocation_limit);
	#endif

	agregroup0_buffer =  (Val*) (((Punt) alloc_base) + per_thread_agegroup0_buffer_bytesize);
    }										// for (int pthread = 0;   pthread < MAX_PTHREADS;   pthread++)
}										// fun partition_agegroup0_buffer_between_pthreads


static volatile int	pthreads_ready_to_clean_local = 0;			// Number of processors that are ready to clean.
static int		cleaning_pthread_local;					// The cleaning pthread.

// This holds extra roots provided by   clean_heap_with_extra_roots:
//
Val*         pth_extra_heapcleaner_roots_global[ MAX_EXTRA_CLEANING_ROOTS * MAX_PTHREADS ];
static Val** mc_extra_cleaner_roots_local;

int   pth_start_heapcleaning   (Task *task) {
    //=================
    //
    // Wait for all pthreads to check in and choose one to do the 
    // collect (cleaning_pthread_local).  cleaning_pthread_local returns to the invoking
    // collection function and does the collect while the other pthreads
    // wait at a barrier. cleaning_pthread_local will eventually check into this
    // barrier releasing the waiting procs.

    int	     nProcs;
    Pthread* pthread = task->pthread;

    pth_acquire_mutex( pth_heapcleaner_mutex_global );

    if (pthreads_ready_to_clean_local++ == 0) {
        //
        pth_extra_heapcleaner_roots_global[0] = NULL;
        mc_extra_cleaner_roots_local = pth_extra_heapcleaner_roots_global;

	#ifdef MULTICORE_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
		ASSIGN( SOFTWARE_GENERATED_PERIODIC_EVENTS_SWITCH_REFCELL_GLOBAL, HEAP_TRUE);
	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
		debug_say ("%d: set poll event\n", task->lib7_mpSelf);
	#endif
	#endif

	cleaning_pthread_local = pthread->pid;			// We're the first one in, we'll do the clean.

	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("cleaning_pthread_local is %d\n",cleaning_pthread_local);
	#endif
    }
    pth_release_mutex(pth_heapcleaner_mutex_global);

    {
	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    int n = 0;
	#endif

        // NB: Some other pthread can be
        // concurrently acquiring new kernel threads.
        //
	while (pthreads_ready_to_clean_local !=  (nProcs = pth_active_pthread_count())) {

	    // SPIN
	    #ifdef NEED_PTHREAD_SUPPORT_DEBUG

		if (n != 10000000) {

		    n++;

		} else {

		    n = 0;

		    debug_say ("%d spinning %d <> %d <alloc=0x%x, limit=0x%x>\n", 
			task->lib7_mpSelf, pthreads_ready_to_clean_local, nProcs, task->heap_allocation_pointer,
			task->heap_allocation_limit);
		}
	    #endif
	}
    }

    // All Pthreads are now ready to clean.

    #ifdef MULTICORE_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	ASSIGN( SOFTWARE_GENERATED_PERIODIC_EVENTS_SWITCH_REFCELL_GLOBAL, HEAP_FALSE);
    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say ("%d: cleared poll event\n", task->lib7_mpSelf);
    #endif
    #endif

    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say ("(%d) all %d/%d procs in\n", task->lib7_mpSelf, pthreads_ready_to_clean_local, pth_active_pthread_count());
    #endif

    if (cleaning_pthread_local != pthread->pid) {

	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("%d entering barrier %d\n",pthread->pid,nProcs);
	#endif

	pth_wait_at_barrier(pth_cleaner_barrier_global, nProcs);
    
	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("%d left barrier\n", pthread->pid);
	#endif

	return 0;
    }

    return nProcs;
}							// fun pth_start_heapcleaning


int   mc_clean_heap_with_extra_roots   (Task *task, va_list ap) {
    //==============================
    //
    // As above, but we collect extra roots into pth_extra_heapcleaner_roots_global.

    int pthread_count;
    Val* p;

    Pthread* pthread =  task->pthread;

    pth_acquire_mutex( pth_heapcleaner_mutex_global );

    if (pthreads_ready_to_clean_local++ == 0) {
	//
        mc_extra_cleaner_roots_local = pth_extra_heapcleaner_roots_global;

	#ifdef MULTICORE_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	    ASSIGN( SOFTWARE_GENERATED_PERIODIC_EVENTS_SWITCH_REFCELL_GLOBAL, HEAP_TRUE);
	    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
		debug_say ("%d: set poll event\n", pthread->pid);
	    #endif
	#endif

	// We're the first one in, we'll do the collect:
	//
	cleaning_pthread_local = pthread->pid;

	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("cleaning_pthread_local is %d\n",cleaning_pthread_local);
	#endif
    }

    while ((p = va_arg(ap, Val *)) != NULL) {
	*mc_extra_cleaner_roots_local++ = p;
    }
    *mc_extra_cleaner_roots_local = p;			// NULL

    pth_release_mutex( pth_heapcleaner_mutex_global );

    {
	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    int n = 0;
	#endif

	// NB: Some other pthread can be concurrently
        // acquiring new kernel threads:
        //
	while (pthreads_ready_to_clean_local !=  (pthread_count = pth_active_pthread_count())) {

	    // SPIN

	    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
		if (n != 10000000) {
		    n++;
		} else {
		    n = 0;
		    debug_say ("%d spinning %d <> %d <alloc=0x%x, limit=0x%x>\n", 
			pthread->pid, pthreads_ready_to_clean_local, pthread_count, task->heap_allocation_pointer,
			task->heap_allocation_limit);
		}
	    #endif
	}
    }

    // All Pthreads now ready to clean:

    #ifdef MULTICORE_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	ASSIGN( SOFTWARE_GENERATED_PERIODIC_EVENTS_SWITCH_REFCELL_GLOBAL, HEAP_FALSE );
    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say ("%d: cleared poll event\n", task->lib7_mpSelf);
    #endif
    #endif

    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say ("(%d) all %d/%d procs in\n", task->pthread->pid, pthreads_ready_to_clean_local, pth_active_pthread_count());
    #endif

    if (cleaning_pthread_local != pthread->pid) {

	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("%d entering barrier %d\n", pthread->pid, pthread_count);
	#endif

	pth_wait_at_barrier(pth_cleaner_barrier_global, pthread_count);
    
	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("%d left barrier\n", pthread->pid);
	#endif

	return 0;
    }

    return  pthread_count;
}							// fun mc_clean_heap_with_extra_roots



void   pth_finish_heapcleaning   (Task *task, int n)   {
    // ==================

    // This works, but partition_agegroup0_buffer_between_pthreads is overkill:		XXX BUGGO FIXME
    //
    partition_agegroup0_buffer_between_pthreads( pthread_table_global );

    pth_acquire_mutex( pth_heapcleaner_mutex_global );

    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say ("%d entering barrier %d\n", task->pthread->pid,n);
    #endif

    pth_wait_at_barrier( pth_cleaner_barrier_global, n );

    pthreads_ready_to_clean_local = 0;

    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say ("%d left barrier\n", task->pthread->pid);
    #endif

    pth_release_mutex(pth_heapcleaner_mutex_global);
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


