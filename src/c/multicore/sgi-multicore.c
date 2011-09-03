// sgi-multicore.c
//
// Multicore (well, multi-processor) support for SGI Challenge machines (Irix 5.x).


#include "../config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <sys/prctl.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <ulocks.h>
#include "runtime-base.h"
#include "runtime-configuration.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "heap-tags.h"
#include "runtime-multicore.h"
#include "task.h"
#include "runtime-globals.h"
#include "pthread.h"

// #define ARENA_FNAME  tmpnam(0)
#define ARENA_FNAME  "/tmp/sml-mp.lock-arena"

#define INT_LIB7inc(n,i)  ((Val)TAGGED_INT_FROM_C_INT(TAGGED_INT_TO_C_INT(n) + (i)))
#define INT_LIB7dec(n,i)  (INT_LIB7inc(n,(-i)))

static Lock      AllocLock ();        
static Barrier*  AllocBarrier();

static usptr_t*	arena;		// Arena for shared sync chunks.
static ulock_t	MP_ArenaLock;	// Must be held to alloc/free a lock.
static ulock_t	MP_ProcLock;	// Must be held to acquire/release procs.

Lock	 mc_cleaner_lock_global;
Lock	 mc_cleaner_gen_lock_global;
Barrier* mc_cleaner_barrier_global;
Lock	 mc_timer_lock_global;



void   mc_initialize   () {
    // =============
    //
    // Called (only) from   src/c/main/runtime-main.c

    // set '_utrace = 1;' to debug shared arenas

    if (usconfig(CONF_LOCKTYPE, US_NODEBUG) == -1)   die ("usconfig failed in mc_initialize");

    usconfig(CONF_AUTOGROW, 0);

    if (usconfig(CONF_INITSIZE, 65536) == -1) 	die ("usconfig failed in mc_initialize");

    if ((arena = usinit(ARENA_FNAME)) == NULL) 	die ("usinit failed in mc_initialize");

    MP_ArenaLock		= AllocLock ();
    MP_ProcLock			= AllocLock ();
    mc_cleaner_lock_global	= AllocLock ();
    mc_cleaner_gen_lock_global	= AllocLock ();
    mc_timer_lock_global	= AllocLock ();
    mc_cleaner_barrier_global	= AllocBarrier();
    //
    ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL, TAGGED_INT_FROM_C_INT(1) );
}



Pid   mc_pthread_id   ()   {
    //=============
    //
    // Called only from:    src/c/main/runtime-state.c
    //
    return getpid ();
}


static Lock   allocate_lock   ()   {
    //        =============
    //
    // Allocate and initialize a system lock.

    ulock_t	lock;

    if ((lock = usnewlock(arena)) == NULL)   die ("allocate_lock: cannot get lock with usnewlock\n");

    usinitlock(lock);
    usunsetlock(lock);

    return lock;
}
 

void   mc_acquire_lock   (Lock lock)   {
    // ===============
    //
    ussetlock(lock);
}


void   mc_release_lock   (Lock lock)   {
    // ===============
    //
    usunsetlock(lock);
}


Bool   mc_try_lock   (Lock lock)   {
    // ===========
    //
    return ((Bool) uscsetlock(lock, 1));		// Try once.
}


Lock   mc_make_lock   ()   {
    // ============
    //
    ulock_t lock;

    ussetlock(   MP_ArenaLock );
        //
	lock = allocate_lock ();
        //
    usunsetlock( MP_ArenaLock );

    return lock;
}



void   mc_free_lock   (Lock lock)   {
    // ============
    //
    ussetlock(MP_ArenaLock);
        usfreelock(lock,arena);
    usunsetlock(MP_ArenaLock);
}


static Barrier*   allocate_barrier   ()   {
     //           ================
     //
     // Allocate and initialize a system barrier.

    barrier_t *barrierp;

    if ((barrierp = new_barrier(arena)) == NULL)   die ("Cannot get barrier with new_barrier");

    init_barrier(barrierp);

    return barrierp;
}
  


Barrier*   mc_make_barrier   ()   {
    //     ===============
    //
    barrier_t *barrierp;

    ussetlock(    MP_ArenaLock );
        //
	barrierp = allocate_barrier ();
        //
    usunsetlock( MP_ArenaLock );

    return barrierp;
}



void   mc_free_barrier   (Barrier* barrierp)   {
    // ===============
    //
    ussetlock(MP_ArenaLock);
	//
	free_barrier( ebarrierp );
	//
    usunsetlock(MP_ArenaLock);
}



void   mc_barrier   (Barrier* barrierp,  unsigned n)   {
    // ==========
    //
    barrier( barrierp, n );
}



void   mc_reset_barrier   (Barrier* barrierp)   {
    // ================
    //
    init_barrier(barrierp);
}


static void   fix_pnum   (int n)   {
    //        ========
    //
    // Dummy for now.
}
 


int   mc_max_pthreads   ()   {
    //===============
    //
    return MAX_PTHREADS;
}



static void   pthread_main   (void* vtask)   {
    //        ============
    //
    Task* task = (Task*) vtask;

// Needs to be done  	XXX BUGGO FIXME
//    fix_pnum(task->pnum);
//    setup_signals(task, TRUE);
//

    // Spin until we get our id (from return of call to make_pthread)
    //
    while (task->pthread->pid == NULL) {
	//
	#ifdef MULTICORE_SUPPORT_DEBUG
	    debug_say("[waiting for self]\n");
	#endif
	continue;
    }
    #ifdef MULTICORE_SUPPORT_DEBUG
	debug_say ("[new proc main: releasing lock]\n");
    #endif

    mc_release_lock( MP_ProcLock );			// Implicitly handed to us by the parent.
    run_mythryl_task_and_runtime_eventloop( task );				// run_mythryl_task_and_runtime_eventloop		def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c
    //
    // run_mythryl_task_and_runtime_eventloop should never return:
    //
    die ("pthread returned after run_mythryl_task_and_runtime_eventloop() in pthread_main().\n");
}



static int   make_pthread   (Task* state)   {
    //       ============
    //
    int error;

    int result = sproc(pthread_main, PR_SALL, (void *)state);

    if (result == -1) {
	extern int errno;

	error = oserror();	// This is potentially a problem since
				// each thread should have its own errno.
				// see sgi man pages for sproc.			XXX BUGGO FIXME

	say_error( "error=%d,errno=%d\n", error, errno );
	say_error( "[warning make_pthread: %s]\n",strerror(error) );
    } 

    return result;
}



Val   mc_acquire_pthread   (Task* task, Val arg)   {
    //==================
    //
    Task* p;
    Pthread* pthread;
    Val v = GET_TUPLE_SLOT_AS_VAL(arg, 0);
    Val f = GET_TUPLE_SLOT_AS_VAL(arg, 1);
    int i;

    #ifdef MULTICORE_SUPPORT_DEBUG
	debug_say("[acquiring proc]\n");
    #endif

    mc_acquire_lock(MP_ProcLock);

    // Search for a suspended kernel thread to reuse:
    //
    for (i = 0;
	(i < pthread_count_global) && (pthread_table_global[i]->status != KERNEL_THREAD_IS_SUSPENDED);
	i++
    ) {
	continue;
    }
    #ifdef MULTICORE_SUPPORT_DEBUG
	debug_say("[checking for suspended processor]\n");
    #endif

    if (i == pthread_count_global) {
        //
	if (DEREF( ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL ) == TAGGED_INT_FROM_C_INT( MAX_PTHREADS )) {
	    //
	    mc_release_lock( MP_ProcLock );
	    say_error("[processors maxed]\n");
	    return HEAP_FALSE;
	}
	#ifdef MULTICORE_SUPPORT_DEBUG
	    debug_say("[checking for NO_PROC]\n");
	#endif

	// Search for a slot in which to put a new proc
	//
	for (i = 0;
	    (i < pthread_count_global) && (pthread_table_global[i]->status != NO_KERNEL_THREAD_ALLOCATED);
	    i++
	){
	    continue;
	}

	if (i == pthread_count_global) {
	    //
	    mc_release_lock(MP_ProcLock);
	    say_error("[no processor to allocate]\n");
	    return HEAP_FALSE;
	}
    }
    #ifdef MULTICORE_SUPPORT_DEBUG
	debug_say("[using processor at index %d]\n", i);
    #endif

    // Use pthread at index i:
    //
    pthread =  pthread_table_global[ i ];

    p =  pthread->task;

    p->exception_fate	=  PTR_CAST( Val,  handle_v + 1 );
    p->argument		=  HEAP_VOID;
    p->fate		=  PTR_CAST( Val, return_c);
    p->closure		=  f;
    p->program_counter	= 
    p->link_register	=  GET_CODE_ADDRESS_FROM_CLOSURE( f );
    p->thread	        =  v;
  
    if (pthread->status == NO_KERNEL_THREAD_ALLOCATED) {
	//
        // Assume we get one:

	ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL, INT_LIB7inc( DEREF(ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL), 1) );

	if ((pthread->pid = make_pthread(p)) != -1) {
	    //
	    #ifdef MULTICORE_SUPPORT_DEBUG
		debug_say ("[got a processor]\n");
	    #endif

	    pthread->status = KERNEL_THREAD_IS_RUNNING;

	    // make_pthread will release MP_ProcLock.

	    return HEAP_TRUE;

	} else {

	    ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL, INT_LIB7dec(DEREF(ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL), 1) );
	    mc_release_lock(MP_ProcLock);
	    return HEAP_FALSE;
	}      

    } else {

	pthread->status = KERNEL_THREAD_IS_RUNNING;

	#ifdef MULTICORE_SUPPORT_DEBUG
	    debug_say ("[reusing a processor]\n");
	#endif

	mc_release_lock(MP_ProcLock);

	return HEAP_TRUE;
    }
}						// fun mc_acquire_pthread



void   mc_release_pthread   (Task* task)   {
    // ==================
    //
    #ifdef MULTICORE_SUPPORT_DEBUG
	debug_say("[release_pthread: suspending]\n");
    #endif

    clean_heap(task,1);

    mc_acquire_lock(MP_ProcLock);

    task->pthread->status = KERNEL_THREAD_IS_SUSPENDED;

    mc_release_lock(MP_ProcLock);

    while (task->pthread->status == KERNEL_THREAD_IS_SUSPENDED) {
	//
        // Need to be continually available for garbage collection:
	//
	clean_heap( task, 1 );
    }
    #ifdef MULTICORE_SUPPORT_DEBUG
	debug_say("[release_pthread: resuming]\n");
    #endif

    run_mythryl_task_and_runtime_eventloop( task );								// run_mythryl_task_and_runtime_eventloop		def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c

    die ("return after run_mythryl_task_and_runtime_eventloop(task) in mp_release_pthread\n");
}



int   mc_active_pthread_count   ()   {
    //=======================
    //
    int ap;

    mc_acquire_lock(MP_ProcLock);
        ap = TAGGED_INT_TO_C_INT( DEREF(ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL) );
    mc_release_lock(MP_ProcLock);

    return ap;
}



void   mc_shut_down   ()   {
    // ============
    //
    usdetach( arena );												// 'usdetach' appears nowhere else in codebase; must be the SGI equivalent to posix 'munmap'
}



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
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


