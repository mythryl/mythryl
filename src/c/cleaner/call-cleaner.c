// call-cleaner.c
//
// The main interface between the heapcleaner
// ("garbage collector") and the rest of the
// run-time system.
//
// We may be manually invoked from the Mythryl level via
//
//     src/lib/std/src/nj/heapcleaner-control.pkg
//
// We may be automatically invoked via the CHECKLIMIT macro in various allocation fns in
//
//     src/c/machine-dependent/prim.intel32.asm
// 
// and thence via   system_run_mythryl_task_and_runtime_eventloop/REQUEST_CLEANING   in
//
//     src/c/main/run-mythryl-code-and-runtime-eventloop.c
//
// to our   clean_heap   entrypoint.
//
// More generally, we may be invoked via the heaplimit checks and heapcleaner calls generated by
//
//     src/lib/compiler/back/low/main/fatecode/pick-fatecode-funs-for-heaplimit-checks.pkg
//     src/lib/compiler/back/low/main/fatecode/insert-treecode-heapcleaner-calls-g.pkg

/*
Includes:
*/
#ifdef KEEP_CLEANER_PAUSE_STATISTICS		// Cleaner pause statistics are UNIX dependent.
    #include "system-dependent-unix-stuff.h"
#endif

#include "../config.h"

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
#include "cleaner-statistics.h"
#include "pthread.h"
#include "profiler-call-counts.h"

#ifdef C_CALLS
    // This is a list of pointers into the C heap locations that hold
    // pointers to Mythryl functions. This list is not part of any Mythryl data
    // package(s).  (also see src/c/cleaner/clean-n-agegroups.c and src/c/lib/ccalls/ccalls-fns.c)
    //
 extern Val	mythryl_functions_referenced_from_c_code_global;	// mythryl_functions_referenced_from_c_code_global	def in   src/c/lib/ccalls/ccalls-fns.c
#endif


void   clean_heap   (Task* task,  int level) {
    // ==========
    //
    // Clean the heap. We always clean agegroup0. If level is greater than
    // 0, or if agegroup0 full after cleaning, we also clean
    // one or more additional agegroups.  (A minimum of 'level' agegroups are cleaned.)

    Val*  roots[ MAX_TOTAL_CLEANING_ROOTS ];						// Registers and globals.
    Val** rootsPtr = roots;
    Heap* heap;

    #ifdef MULTICORE_SUPPORT
	int		pthreads_count;
    #endif

    ASSIGN( THIS_FN_PROFILING_HOOK_REFCELL_GLOBAL, PROF_MINOR_CLEANING );				// THIS_FN_PROFILING_HOOK_REFCELL_GLOBAL is #defined      in	src/c/h/runtime-globals.h
												//  in terms of   this_fn_profiling_hook_refcell_global   from	src/c/main/construct-runtime-package.c

    #ifdef MULTICORE_SUPPORT
    #ifdef MULTICORE_SUPPORT_DEBUG
	debug_say ("igc %d\n", task->lib7_mpSelf);
    #endif
	if ((pthreads_count = mc_start_cleaning (task)) == 0) {
	    //
	    // A waiting proc:
	    //
	    ASSIGN( THIS_FN_PROFILING_HOOK_REFCELL_GLOBAL, PROF_RUNTIME );
	    return;
	}
    #endif

    note_when_cleaning_started( task->heap );						// note_when_cleaning_started	def in    src/c/cleaner/cleaner-statistics.h

    #ifdef C_CALLS
	*rootsPtr++ = &mythryl_functions_referenced_from_c_code_global;
    #endif

    #ifdef MULTICORE_SUPPORT
	// Get extra roots from procs that entered
	// through clean_heap_with_extra_roots
	//
	for (int i = 0;   mc_extra_cleaner_roots_global[i] != NULL;   i++) {
	    //
	    *rootsPtr++ =  mc_extra_cleaner_roots_global[i];
	}
    #endif

    // Gather ye roots while you may:
    //
    for (int i = 0;  i < c_roots_count_global;  i++)   {
	//
	*rootsPtr++ = c_roots_global[ i ];
    }

    #ifdef MULTICORE_SUPPORT
	{
	    Pthread* pthread;
	    Task*	 task;

	    for (int j = 0;  j < MAX_PTHREADS;  j++) {
		//
		pthread = pthread_table_global[ j ];

		task = pthread->task;

		#ifdef MULTICORE_SUPPORT_DEBUG
		    debug_say ("task[%d] alloc/limit was %x/%x\n", j, task->heap_allocation_pointer, task->heap_allocation_limit);
		#endif

		if (pthread->status == KERNEL_THREAD_IS_RUNNING) {
		    //
		    *rootsPtr++ =  &task->argument;
		    *rootsPtr++ =  &task->fate;
		    *rootsPtr++ =  &task->closure;
		    *rootsPtr++ =  &task->exception_fate;
		    *rootsPtr++ =  &task->thread;
		    *rootsPtr++ =  &task->callee_saved_registers[0];
		    *rootsPtr++ =  &task->callee_saved_registers[1];
		    *rootsPtr++ =  &task->callee_saved_registers[2];
		}
	    }
	}
    #else								// !MULTICORE_SUPPORT
	//	
	*rootsPtr++ =  &task->link_register;
	*rootsPtr++ =  &task->argument;
	*rootsPtr++ =  &task->fate;
	*rootsPtr++ =  &task->closure;
	*rootsPtr++ =  &task->exception_fate;
	*rootsPtr++ =  &task->thread;
	*rootsPtr++ =  &task->callee_saved_registers[0];
	*rootsPtr++ =  &task->callee_saved_registers[1];
	*rootsPtr++ =  &task->callee_saved_registers[2];
    #endif										// MULTICORE_SUPPORT

    *rootsPtr = NULL;

    clean_agegroup0( task, roots );							// clean_agegroup0	is from   src/c/cleaner/clean-agegroup0.c

    heap = task->heap;

    // Check for full cleaning:
    //
    if (level == 0) {
        //
	Agegroup*	age1 =  heap->agegroup[0];
        //
	Val_Sized_Unt	size =   task->heap->agegroup0_buffer_bytesize;

	for (int i = 0;  i < MAX_PLAIN_ILKS;  i++) {
	    //
	    Sib* sib =  age1->sib[ i ];

	    if (sib_is_active( sib )							// sib_is_active		def in    src/c/h/heap.h
            &&  sib_freespace_in_bytes( sib ) < size					// sib_freespace_in_bytes	def in    src/c/h/heap.h
            ){
		level = 1;
		break;
	    }
	}
    }

    if (level > 0) {

	#ifdef MULTICORE_SUPPORT
            //	
	    Task* task;

	    for (int i = 0;  i < MAX_PTHREADS;  i++) {
		//
		Pthread*  pthread =  pthread_table_global[ i ];
		//
		task  =  pthread->task;
		//
		if (pthread->status == KERNEL_THREAD_IS_RUNNING) {
		    //
		    *rootsPtr++ =  &task->link_register;
		}
	    }
	#else
	    ASSIGN( THIS_FN_PROFILING_HOOK_REFCELL_GLOBAL, PROF_MAJOR_CLEANING );
	#endif

	*rootsPtr = NULL;

	clean_n_agegroups( task, roots, level );							// clean_n_agegroups			def in   src/c/cleaner/clean-n-agegroups.c
    }

    // Reset the allocation space:
    //
    #ifdef MULTICORE_SUPPORT
	mc_finish_cleaning( task, pthreads_count );
    #else
	task->heap_allocation_pointer	= heap->agegroup0_buffer;

	#ifdef SOFTWARE_GENERATED_PERIODIC_EVENTS
	    reset_heap_allocation_limit_for_software_generated_periodic_events( task );
	#else
	    task->heap_allocation_limit    = HEAP_ALLOCATION_LIMIT( heap );
	#endif
    #endif

    note_when_cleaning_completed();									// note_when_cleaning_completed	def in    src/c/cleaner/cleaner-statistics.h

    ASSIGN( THIS_FN_PROFILING_HOOK_REFCELL_GLOBAL, PROF_RUNTIME );
}			                                             // fun clean_heap


void   clean_heap_with_extra_roots   (Task* task,  int level, ...)   {
    // ===========================
    //
    // Clean with possible additional roots.  The list of
    // additional roots is NULL terminated.  We always clean agegroup0.
    // If level is greater than 0, or if agegroup 1 is full after cleaning
    // agegroup0, then we clean one or more additional agegroups.
    // At least 'level' agegroups are cleaned.
    //
    // NOTE: the multicore version of this may be BROKEN, since if a processor calls this
    // but isn't the collecting process, then THE EXTRA ROOTS ARE LOST.  XXX BUGGO FIXME

    Val*  roots[ MAX_TOTAL_CLEANING_ROOTS + MAX_EXTRA_CLEANING_ROOTS ];	// registers and globals
    Val** rootsPtr = roots;
    Val*  p;
    Heap* heap;

    va_list ap;

    #ifdef MULTICORE_SUPPORT
	int pthreads_count;
    #endif

    ASSIGN( THIS_FN_PROFILING_HOOK_REFCELL_GLOBAL, PROF_MINOR_CLEANING );

    #ifdef MULTICORE_SUPPORT

	#ifdef MULTICORE_SUPPORT_DEBUG
	    debug_say ("igcwr %d\n", task->lib7_mpSelf);
	#endif

	va_start (ap, level);
	pthreads_count = mc_clean_heap_with_extra_roots (task, ap);
	va_end(ap);

	if (pthreads_count == 0)	ASSIGN( THIS_FN_PROFILING_HOOK_REFCELL_GLOBAL, PROF_RUNTIME );

	return;				// A waiting proc
    #endif

    note_when_cleaning_started( task->heap );								// note_when_cleaning_started	def in    src/c/cleaner/cleaner-statistics.h

    #ifdef C_CALLS
	*rootsPtr++ = &mythryl_functions_referenced_from_c_code_global;
    #endif

    #ifdef MULTICORE_SUPPORT
        // get extra roots from procs that entered through clean_heap_with_extra_roots.
        // Our extra roots were placed in mc_extra_cleaner_roots_global by mc_clean_heap_with_extra_roots.
        //
	for (int i = 0;  mc_extra_cleaner_roots_global[i] != NULL;  i++) {
	    //
	    *rootsPtr++ = mc_extra_cleaner_roots_global[i];
	}
    #else
        // Record extra roots from param list:
	va_start (ap, level);
	while ((p = va_arg(ap, Val *)) != NULL) {
	    *rootsPtr++ = p;
	}
	va_end(ap);
    #endif						// MULTICORE_SUPPORT

    // Gather the roots:
    //
    for (int i = 0;  i < c_roots_count_global;  i++) {
	*rootsPtr++ = c_roots_global[i];
    }

    #ifdef MULTICORE_SUPPORT
	{
	    Task*     task;
	    Pthread*  pthread;

	    for (int j = 0;  j < MAX_PTHREADS;  j++) {
		//
		pthread = pthread_table_global[ j ];
		task    = pthread->task;

		#ifdef MULTICORE_SUPPORT_DEBUG
		    debug_say ("task[%d] alloc/limit was %x/%x\n",
			    j, task->heap_allocation_pointer, task->heap_allocation_limit);
		#endif

		if (pthread->status == KERNEL_THREAD_IS_RUNNING) {
		    //
		    *rootsPtr++ =  &task->argument;
		    *rootsPtr++ =  &task->fate;
		    *rootsPtr++ =  &task->closure;
		    *rootsPtr++ =  &task->exception_fate;
		    *rootsPtr++ =  &task->thread;
		    *rootsPtr++ =  &task->callee_saved_registers[ 0 ];
		    *rootsPtr++ =  &task->callee_saved_registers[ 1 ];
		    *rootsPtr++ =  &task->callee_saved_registers[ 2 ];
		}
	    }
	}

    #else						// !MULTICORE_SUPPORT

	*rootsPtr++ =  &task->argument;
	*rootsPtr++ =  &task->fate;
	*rootsPtr++ =  &task->closure;
	*rootsPtr++ =  &task->exception_fate;
	*rootsPtr++ =  &task->thread;
	*rootsPtr++ =  &task->callee_saved_registers[0];
	*rootsPtr++ =  &task->callee_saved_registers[1];
	*rootsPtr++ =  &task->callee_saved_registers[2];

    #endif						// MULTICORE_SUPPORT

    *rootsPtr = NULL;

    clean_agegroup0( task, roots );			// clean_agegroup0	is from   src/c/cleaner/clean-agegroup0.c

    heap = task->heap;

    // Check for major GC:
    //
    if (level == 0) {
        //
	Agegroup*	age1 =  heap->agegroup[0];
        //
	Val_Sized_Unt  size =  task->heap->agegroup0_buffer_bytesize;

	for (int i = 0;  i < MAX_PLAIN_ILKS;  i++) {
	    //
	    Sib* sib = age1->sib[ i ];
	    //
	    if (sib_is_active( sib )							// sib_is_active		def in    src/c/h/heap.h
            && (sib_freespace_in_bytes( sib ) < size)					// sib_freespace_in_bytes	def in    src/c/h/heap.h
            ){
		level = 1;
		break;
	    }
	}
    }

    if (level > 0) {
	//
	#ifdef MULTICORE_SUPPORT
	    //
	    Pthread* pthread;

	    for (int i = 0;  i < MAX_PTHREADS;  i++) {
	        //
		pthread = pthread_table_global[ i ];
		//
		if (pthread->status == KERNEL_THREAD_IS_RUNNING) {
		    //
		    *rootsPtr++ =  &pthread->task->link_register;
		}
	    }
	#else
	    ASSIGN( THIS_FN_PROFILING_HOOK_REFCELL_GLOBAL, PROF_MAJOR_CLEANING );
	    //
	    *rootsPtr++ =  &task->link_register;
	    *rootsPtr++ =  &task->program_counter;
	#endif

	*rootsPtr = NULL;

	clean_n_agegroups( task, roots, level );								// clean_n_agegroups			def in   src/c/cleaner/clean-n-agegroups.c

    }

    // Reset agegroup0 buffer:
    //
    #ifdef MULTICORE_SUPPORT
        //
	mc_finish_cleaning (task, pthreads_count);
    #else
	task->heap_allocation_pointer	= heap->agegroup0_buffer;

	#ifdef SOFTWARE_GENERATED_PERIODIC_EVENTS
	    reset_heap_allocation_limit_for_software_generated_periodic_events( task );
	#else
	    task->heap_allocation_limit    = HEAP_ALLOCATION_LIMIT(heap);
	#endif
    #endif

    note_when_cleaning_completed();										// note_when_cleaning_completed	def in    src/c/cleaner/cleaner-statistics.h

    ASSIGN( THIS_FN_PROFILING_HOOK_REFCELL_GLOBAL, PROF_RUNTIME );
}														// fun clean_heap_with_extra_roots



Bool   need_to_clean_heap   (Task* task,  Val_Sized_Unt nbytes)   {
    // ==================
    //
    // Check to see if a GC is required,
    // or if there is enough heap space
    // for nbytes worth of allocation.
    //	
    // Return TRUE, if GC is required,
    // FALSE otherwise.

    #if (defined(MULTICORE_SUPPORT) && defined(COMMENT_MULTICORE_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS))
	//
	if ((((Punt)(task->heap_allocation_pointer)+nbytes) >= (Punt) task->heap_allocation_limit)
	|| (INT31_TO_C_INT( SOFTWARE_GENERATED_PERIODIC_EVENTS_SWITCH_REFCELL_GLOBAL) != 0))
	//
    #elif defined(MULTICORE_SUPPORT)
	//
	if (((Punt)(task->heap_allocation_pointer)+nbytes) >= (Punt) task->heap_allocation_limit)
	//
    #else
	if (((Punt)(task->heap_allocation_pointer)+nbytes) >= (Punt) HEAP_ALLOCATION_LIMIT( task->heap ))
    #endif
	return TRUE;
    else
	return FALSE;
}


#ifdef SOFTWARE_GENERATED_PERIODIC_EVENTS

    void   reset_heap_allocation_limit_for_software_generated_periodic_events   (Task* task)   {
	// =======================================================
	//
	// Reset the limit pointer according to the current polling frequency.

	int poll_frequency
	    = 
	    INT31_TO_C_INT(DEREF(SOFTWARE_GENERATED_PERIODIC_EVENT_INTERVAL_REFCELL_GLOBAL));	// SOFTWARE_GENERATED_PERIODIC_EVENT_INTERVAL_REFCELL_GLOBAL is #defined in src/c/h/runtime-globals.h
											    	// in terms of software_generated_periodic_event_interval_refcell_global from src/c/main/construct-runtime-package.c
	Heap* heap =  task->heap;

	// Assumes heap_allocation_pointer has been reset:
	//
	task->real_heap_allocation_limit =  HEAP_ALLOCATION_LIMIT( heap );

	if (poll_frequency <= 0) {
	    //
	    task->heap_allocation_limit  = task->real_heap_allocation_limit;

	} else {

	    task->heap_allocation_limit  =  heap->allocBase + poll_frequency * PERIODIC_EVENT_TIME_GRANULARITY_IN_FATECODE_INSTRUCTIONS;
	    //
	    task->heap_allocation_limit  =  (task->heap_allocation_limit > task->real_heap_allocation_limit)
		? task->real_heap_allocation_limit
		: task->heap_allocation_limit;
	}
    }
#endif						// SOFTWARE_GENERATED_PERIODIC_EVENTS


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
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
