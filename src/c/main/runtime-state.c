// runtime-state.c

#include "../mythryl-config.h"

#include <stdarg.h>
#include <string.h>
#include "runtime-base.h"
#include "heap-tags.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-globals.h"
#include "heapcleaner.h"
#include "runtime-timer.h"
#include "runtime-configuration.h"


												// struct hostthread { 				def in   src/c/h/runtime-base.h
												// typedef struct hostthread	Hostthread;	def in   src/c/h/runtime-base.h
Hostthread*   hostthread_table__global[  MAX_HOSTTHREADS  ];					// hostthread_table__global[] is exported	via      src/c/h/runtime-base.h
    //     =====================
    //
    // Table of all active posix threads in process.
    // (Or at least, all posix threads running Mythryl
    // code or accessing the Mythryl heap.)
    //
    // This table is initialized by   make_task()   (below).
    //
    // In multithreaded operation this table is modified
    // by the heapcleaner -- of course -- but otherwise
    // only by code in   src/c/hostthread/hostthread-on-posix-threads.c
    // serialized by the hostthread_table_mutex__local.


static void   set_up_hostthread_state   (Hostthread* hostthread);

Task*   make_task   (Bool is_boot,  Heapcleaner_Args* cleaner_args)    {
    //  =========
    //
    // This function is called two places, one each in:
    //
    //     src/c/heapcleaner/import-heap.c
    //     src/c/main/load-compiledfiles.c

    Task* task =  NULL;

    //
    for (int i = 0;   i < MAX_HOSTTHREADS;   i++) {
	//
	if (((hostthread_table__global[i] = MALLOC_CHUNK(Hostthread)) == NULL)
	||  ((task = MALLOC_CHUNK(Task)) == NULL)
	){
	    die ("runtime-state.c: unable to allocate hostthread_table__global entry");
	}

	hostthread_table__global[i]->task =  task;
    }
    task =  hostthread_table__global[0]->task;

    // Allocate and initialize the heap data structures:
    //
    set_up_heap( task, is_boot, cleaner_args );							// set_up_heap					def in    src/c/heapcleaner/heapcleaner-initialization.c

    // 'set_up_heap' has created an agegroup0 buffer;
    //  partition it between our MAX_HOSTTHREADS hostthreads:
    //
    partition_agegroup0_buffer_between_hostthreads( hostthread_table__global );			// partition_agegroup0_buffer_between_hostthreads	def in   src/c/heapcleaner/hostthread-heapcleaner-stuff.c

    // Initialize the per-Hostthread Mythryl state:
    //
    for (int i = 0;  i < MAX_HOSTTHREADS;  i++) {
	//
	hostthread_table__global[i]->id   =  i;							// pth__get_hostthread_ptid () returns huge numbers, this gives us small hostthread ids.

	set_up_hostthread_state( hostthread_table__global[i] );

	// Single timers are currently shared
	// among multiple hostthreads:
	//
	if (i != 0) {
	    hostthread_table__global[ i ] -> cpu_time_at_start_of_last_heapclean
	  = hostthread_table__global[ 0 ] -> cpu_time_at_start_of_last_heapclean;
	    //
	    hostthread_table__global[ i ] -> cumulative_cleaning_cpu_time
	  = hostthread_table__global[ 0 ] -> cumulative_cleaning_cpu_time;
	}
    }

    // Initialize the first Hostthread here:
    //
    hostthread_table__global[0]->ptid =  pth__get_hostthread_ptid ();				// pth__get_hostthread_ptid				def in    src/c/hostthread/hostthread-on-posix-threads.c
    hostthread_table__global[0]->mode =  HOSTTHREAD_IS_RUNNING;
    hostthread_table__global[0]->name =  "default";

    // Initialize the timers:
    //
    reset_timers( hostthread_table__global[0] );						// "Hostthread support note: For now, only Hostthread 0 has timers." (Ancient comment, not sure it is true. -- 2012-03-02 CrT)

    return task;
}							// fun make_task



static void   set_up_hostthread_state   (Hostthread* hostthread)   {
    //        ====================
    //
    hostthread->heap				= hostthread->task->heap;
    hostthread->task->hostthread		= hostthread;
    //
    hostthread->executing_mythryl_code		= FALSE;
    hostthread->interprocess_signal_pending	= FALSE;
    hostthread->mythryl_handler_for_interprocess_signal_is_running		= FALSE;
    //
    hostthread->all_posix_signals.seen_count	= 0;
    hostthread->all_posix_signals.done_count	= 0;
    hostthread->next_posix_signal_id		= 0;
    hostthread->next_posix_signal_count		= 0;
    //
    hostthread->posix_signal_rotor		= 0;
    //
    hostthread->cpu_time_at_start_of_last_heapclean		= MALLOC_CHUNK(Time);
    hostthread->cumulative_cleaning_cpu_time			= MALLOC_CHUNK(Time);

    clear_signal_counts( hostthread );								// clear_signal_counts()	is from   src/c/machine-dependent/interprocess-signals.c

    // Initialize the Mythryl state, including the roots:
    //
    initialize_task( hostthread->task );
    //
    hostthread->task->argument			= HEAP_VOID;
    hostthread->task->fate			= HEAP_VOID;
    hostthread->task->current_closure		= HEAP_VOID;
    hostthread->task->link_register		= HEAP_VOID;
    hostthread->task->program_counter		= HEAP_VOID;
    hostthread->task->exception_fate		= HEAP_VOID;
    hostthread->task->current_thread		= HEAP_VOID;
    hostthread->task->callee_saved_registers[0]	= HEAP_VOID;
    hostthread->task->callee_saved_registers[1]	= HEAP_VOID;
    hostthread->task->callee_saved_registers[2]	= HEAP_VOID;
    hostthread->task->heapvoid			= HEAP_VOID;			// Something for protected_c_arg to point to when not being used.
    hostthread->task->protected_c_arg		= &hostthread->task->heapvoid;	// Support for  RELEASE_MYTHRYL_HEAP  in  src/c/h/runtime-base.h

    hostthread->ptid		= 0;						// Note that '0' works as an initializer whether pthread_t is an int or pointer on the host OS.
    hostthread->mode		= HOSTTHREAD_IS_VOID;
    hostthread->name		= "(anonymous)";
}										// fun set_up_hostthread_state

void   initialize_task   (Task* task)   {
    // ===============
    //
    // Initialize the Mythryl state vector.
    //
    // Note that we do not initialize the root registers here,
    // since this is sometimes called when the roots are live
    // (from run_mythryl_function).					// run_mythryl_function__may_heapclean		def in    src/c/main/run-mythryl-code-and-runtime-eventloop.c
    //
    // This fn is called two places, above and in
    //     src/c/main/run-mythryl-code-and-runtime-eventloop.c

    task->heap_changelog =   HEAP_VOID;

    #if NEED_SOFTWARE_GENERATED_PERIODIC_EVENTS
        //
	task->software_generated_periodic_event_is_pending	= FALSE;
	task->in_software_generated_periodic_event_handler	= FALSE;
    #endif
}

void   save_c_state   (Task* task, Roots* extra_roots)   {
    // ============
    // 
    // Build a return closure that will save
    // a collection of Mythryl values being
    // used by C.  The Mythryl values are
    // passed by reference with NULL as termination.
    //
    // This fn is called only in:
    //
    //     src/c/main/load-compiledfiles.c

    int  n = 0;

    Roots* r;
    for (r = extra_roots;   r;   r = r->next)  ++n;

    set_slot_in_nascent_heapchunk (task, 0, MAKE_TAGWORD(n, PAIRS_AND_RECORDS_BTAG));
    int  i = 1;
    for (r = extra_roots;   r;   r = r->next, ++i) {
	//
        set_slot_in_nascent_heapchunk (task, i, *r->root);
    }    
    task->callee_saved_registers[0] =  commit_nascent_heapchunk(task, n);

    task->fate =  PTR_CAST( Val, return_to_c_level_c);
}

void   restore_c_state   (Task* task, Roots* extra_roots)   {
    // ===============
    //
    //    Restore a collection of Mythryl values from the return closure.
    //
    // This fn is called only in:
    //
    //     src/c/main/load-compiledfiles.c

    Val*    vp;
    Val	    saved_state;

    saved_state = task->callee_saved_registers[0];

    int n							/* This variable will be unused if ASSERTs, are off, so suppress 'unused var' compiler warning: */   		__attribute__((unused))
        =
        CHUNK_LENGTH(saved_state);

    int  i = 0;

    for (Roots* r = extra_roots;   r;   r = r->next, ++i) {
	//
	vp =  r->root;
       *vp =  GET_TUPLE_SLOT_AS_VAL(saved_state, i);
    }

    ASSERT( i == n );
}


///////////////////////////////////////////////////////////////////////////
// Support for RELEASE_MYTHRYL_HEAP.
//
// See overview comments in   src/c/h/runtime-base.h
//
// (This code should maybe have its own .c file.)
//
void*   buffer_mythryl_heap_value(
	    //
	    Mythryl_Heap_Value_Buffer*	buf,									// Mythryl_Heap_Value_Buffer				def in   src/c/h/runtime-base.h
	    void*			heapval,
	    int				heapval_bytesize
	)
{
    // For speed, we buffer small values on the stack:
    //
    if (heapval_bytesize < MAX_STACK_BUFFERED_MYTHRYL_HEAP_VALUE) {						// A few KB:  MAX_STACK_BUFFERED_MYTHRYL_HEAP_VALUE	def in   src/c/h/runtime-base.h
        //
	buf->heap_space = NULL;											// Make sure this is initialized -- we'll call free() on this in buffer_mythryl_heap_value().
	//
	memcpy( buf->stack_space, heapval, heapval_bytesize );
	return  buf->stack_space;
	//
    } else {
        //
        // Larger values we buffer on the heap.
        // Copying heapval will probably take longer
        // than the malloc() call anyhow, in this size range:
        //
	buf->heap_space =  malloc( heapval_bytesize );
        //
	if (!buf->heap_space)  die( "buffer_mythryl_heap_value: Unable to malloc(%d)\n", heapval_bytesize ); 
        //
	memcpy( buf->heap_space, heapval, heapval_bytesize );
	return  buf->heap_space;
    }
}

void*   buffer_mythryl_heap_nonvalue(										// Same as above, except we're just providing space, not copying anything into it.
	    //
	    Mythryl_Heap_Value_Buffer*	buf,									// Mythryl_Heap_Value_Buffer				def in   src/c/h/runtime-base.h
	    int				bytes
	)
{
    // For speed, we buffer small values on the stack:
    //
    if (bytes < MAX_STACK_BUFFERED_MYTHRYL_HEAP_VALUE) {							// A few KB:  MAX_STACK_BUFFERED_MYTHRYL_HEAP_VALUE	def in   src/c/h/runtime-base.h
        //
	buf->heap_space = NULL;											// Make sure this is initialized -- we'll call free() on this in buffer_mythryl_heap_value().
	//
	return  buf->stack_space;
	//
    } else {
        //
        // Larger values we buffer on the heap.
        // Copying heapval will probably take longer
        // than the malloc() call anyhow, in this size range:
        //
	buf->heap_space =  malloc( bytes );
        //
	if (!buf->heap_space)  die( "buffer_mythryl_heap_value: Unable to malloc(%d)\n", bytes ); 
        //
	return  buf->heap_space;
    }
}

void   unbuffer_mythryl_heap_value(   Mythryl_Heap_Value_Buffer* buf   ) {					// Mythryl_Heap_Value_Buffer				def in   src/c/h/runtime-base.h
    //
    free( buf->heap_space );											// It is ok to call free(NULL).
}



// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


