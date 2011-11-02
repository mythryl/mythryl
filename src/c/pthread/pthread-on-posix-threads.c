// pthread-on-posix-threads.c
//
// This is an ancient (1994?) implementation of pthread support on top of the
// SGI Challenge boxes of the era which featured up to sixteen CPU cards on a
// single bus with a special dedicated hardware bus for inter-CPU locking etc.
//
// Posix-threads based implementation of the API defined in
//
//     src/c/h/runtime-pthread.h
//
// parts of which are exported to the Mythryl level via
//
//     src/c/lib/pthread/libmythryl-pthread.c
// 
// and then
// 
//     src/lib/std/src/pthread.api
//     src/lib/std/src/pthread.pkg


#include "../mythryl-config.h"

#include <pthread.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <sys/prctl.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "runtime-base.h"
#include "runtime-configuration.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "heap-tags.h"
#include "runtime-pthread.h"
#include "task.h"
#include "runtime-globals.h"
#include "pthread-state.h"

Pid   pth_get_pthread_id   ()   {
    //==================
    //
    return getpid ();
}

#ifdef SOON

// #define ARENA_FNAME  tmpnam(0)
#define ARENA_FNAME  "/tmp/sml-mp.mutex-arena"

#define INT_LIB7inc(n,i)  ((Val)TAGGED_INT_FROM_C_INT(TAGGED_INT_TO_C_INT(n) + (i)))
#define INT_LIB7dec(n,i)  (INT_LIB7inc(n,(-i)))

static Mutex      AllocLock ();        
static Barrier*  AllocBarrier();

static usptr_t*	arena;								// Arena for shared sync chunks.

static ulock_t	MP_ArenaLock;							// Must be held to alloc/free a mutex.

static ulock_t	MP_ProcLock;							// Must be held to acquire/release procs.

Mutex	 pth_heapcleaner_mutex_global;						// Used only in   src/c/heapcleaner/pthread-heapcleaner-stuff.c

Mutex	 pth_heapcleaner_gen_mutex_global;						// Used only in   src/c/heapcleaner/make-strings-and-vectors-etc.c

Barrier* pth_cleaner_barrier_global;						// Used only with pth_wait_at_barrier prim, in   src/c/heapcleaner/pthread-heapcleaner-stuff.c

Mutex	 pth_timer_mutex_global;							// Apparently never used.



void   pth_initialize   () {
    // =============
    //
    // Called (only) from   src/c/main/runtime-main.c

    // set '_utrace = 1;' to debug shared arenas

    if (usconfig(CONF_LOCKTYPE, US_NODEBUG) == -1)   die ("usconfig failed in pth_initialize");

    usconfig(CONF_AUTOGROW, 0);

    if (usconfig(CONF_INITSIZE, 65536) == -1) 	die ("usconfig failed in pth_initialize");

    if ((arena = usinit(ARENA_FNAME)) == NULL) 	die ("usinit failed in pth_initialize");

    MP_ArenaLock		= AllocLock ();
    MP_ProcLock			= AllocLock ();
    pth_heapcleaner_mutex_global	= AllocLock ();
    pth_heapcleaner_gen_mutex_global	= AllocLock ();
    pth_timer_mutex_global	= AllocLock ();
    pth_cleaner_barrier_global	= AllocBarrier();
    //
    ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL, TAGGED_INT_FROM_C_INT(1) );
}

void   pth_shut_down   ()   {
    // =============
    //
    usdetach( arena );												// 'usdetach' appears nowhere else in codebase; must be the SGI equivalent to posix 'munmap'
}


Pid   pth_pthread_id   ()   {
    //=============
    //
    // Called only from:    src/c/main/runtime-state.c
    //
    return getpid ();
}


static Mutex   allocate_mutex   ()   {
    //        ==============
    //
    // Allocate and initialize a system mutex.

    ulock_t	mutex;

    if ((mutex = usnewlock(arena)) == NULL)   die ("allocate_mutex: cannot get mutex with usnewlock\n");

    usinitlock(mutex);
    usunsetlock(mutex);

    return mutex;


}
 

void   pth_acquire_mutex   (Mutex mutex)   {
    // =================
    //
    ussetlock(mutex);
}


void   pth_release_mutex   (Mutex mutex)   {
    // =================
    //
    usunsetlock(mutex);
}


Bool   pth_maybe_acquire_mutex   (Mutex mutex)   {
    // =======================
    //
    return ((Bool) uscsetlock(mutex, 1));		// Try once.
}


Mutex   pth_make_mutex   ()   {
    // ==============
    //
    ulock_t mutex;

    ussetlock(   MP_ArenaLock );
        //
	mutex = allocate_mutex ();
        //
    usunsetlock( MP_ArenaLock );

    return mutex;
}



void   pth_free_mutex   (Mutex mutex)   {
    // =============
    //
    ussetlock(MP_ArenaLock);
        usfreelock(mutex,arena);
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
  


Barrier*   pth_make_barrier   ()   {
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



void   pth_free_barrier   (Barrier* barrierp)   {
    // ===============
    //
    ussetlock(MP_ArenaLock);
	//
	free_barrier( ebarrierp );
	//
    usunsetlock(MP_ArenaLock);
}



void   pth_wait_at_barrier   (Barrier* barrierp,  unsigned n)   {
    // ==========
    //
    barrier( barrierp, n );
}



void   pth_reset_barrier   (Barrier* barrierp)   {
    // ================
    //
    init_barrier(barrierp);
}


static void   fix_pnum   (int n)   {
    //        ========
    //
    // Dummy for now.
}
 


int   pth_max_pthreads   ()   {						// This fn gets exported to the Mythryl level; not used at the C level.
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
	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say("[waiting for self]\n");
	#endif
	continue;
    }
    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say ("[new proc main: releasing mutex]\n");
    #endif

    pth_release_mutex( MP_ProcLock );			// Implicitly handed to us by the parent.
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



Val   pth_acquire_pthread   (Task* task, Val arg)   {
    //==================
    //
    Task* p;
    Pthread* pthread;

    Val thread_arg  =  GET_TUPLE_SLOT_AS_VAL( arg, 0 );			// This is stored into   pthread->task->thread.
    Val closure_arg =  GET_TUPLE_SLOT_AS_VAL( arg, 1 );			// This is stored into   pthread->task->closure
									// and also              pthread->task->link.
    int i;

    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say("[acquiring proc]\n");
    #endif

    pth_acquire_mutex(MP_ProcLock);

    // Search for a suspended kernel thread to reuse:
    //
    for (i = 0;
	(i < pthread_count_global) && (pthread_table_global[i]->status != PTHREAD_IS_SUSPENDED);
	i++
    ) {
	continue;
    }
    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say("[checking for suspended processor]\n");
    #endif

    if (i == pthread_count_global) {
        //
	if (DEREF( ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL ) == TAGGED_INT_FROM_C_INT( MAX_PTHREADS )) {
	    //
	    pth_release_mutex( MP_ProcLock );
	    say_error("[processors maxed]\n");
	    return HEAP_FALSE;
	}
	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say("[checking for NO_PROC]\n");
	#endif

	// Search for a slot in which to put a new proc
	//
	for (i = 0;
	    (i < pthread_count_global) && (pthread_table_global[i]->status != NO_PTHREAD_ALLOCATED);
	    i++
	){
	    continue;
	}

	if (i == pthread_count_global) {
	    //
	    pth_release_mutex(MP_ProcLock);
	    say_error("[no processor to allocate]\n");
	    return HEAP_FALSE;
	}
    }
    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say("[using processor at index %d]\n", i);
    #endif

    // Use pthread at index i:
    //
    pthread =  pthread_table_global[ i ];

    p =  pthread->task;

    p->exception_fate	=  PTR_CAST( Val,  handle_v + 1 );
    p->argument		=  HEAP_VOID;
    p->fate		=  PTR_CAST( Val, return_c);
    p->closure		=  closure_arg;
    p->program_counter	= 
    p->link_register	=  GET_CODE_ADDRESS_FROM_CLOSURE( closure_arg );
    p->thread	        =  thread_arg;
  
    if (pthread->status == NO_PTHREAD_ALLOCATED) {
	//
        // Assume we get one:

	ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL, INT_LIB7inc( DEREF(ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL), 1) );

	if ((pthread->pid = make_pthread(p)) != -1) {
	    //
	    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
		debug_say ("[got a processor]\n");
	    #endif

	    pthread->status = PTHREAD_IS_RUNNING;

	    // make_pthread will release MP_ProcLock.

	    return HEAP_TRUE;

	} else {

	    ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL, INT_LIB7dec(DEREF(ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL), 1) );
	    pth_release_mutex(MP_ProcLock);
	    return HEAP_FALSE;
	}      

    } else {

	pthread->status = PTHREAD_IS_RUNNING;

	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("[reusing a processor]\n");
	#endif

	pth_release_mutex(MP_ProcLock);

	return HEAP_TRUE;
    }
}						// fun pth_acquire_pthread



void   pth_release_pthread   (Task* task)   {
    // ==================
    //
    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say("[release_pthread: suspending]\n");
    #endif

    clean_heap(task,1);

    pth_acquire_mutex(MP_ProcLock);

    task->pthread->status = PTHREAD_IS_SUSPENDED;

    pth_release_mutex(MP_ProcLock);

    while (task->pthread->status == PTHREAD_IS_SUSPENDED) {
	//
        // Need to be continually available for garbage collection:
	//
	clean_heap( task, 1 );
    }
    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say("[release_pthread: resuming]\n");
    #endif

    run_mythryl_task_and_runtime_eventloop( task );								// run_mythryl_task_and_runtime_eventloop		def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c

    die ("return after run_mythryl_task_and_runtime_eventloop(task) in mp_release_pthread\n");
}



int   pth_active_pthread_count   ()   {
    //========================
    //
    int ap;

    pth_acquire_mutex(MP_ProcLock);
        ap = TAGGED_INT_TO_C_INT( DEREF(ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL) );
    pth_release_mutex(MP_ProcLock);

    return ap;
}



#endif


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


