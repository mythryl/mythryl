// pthread-on-sgi.c
//
// This is an ancient (1994?) implementation of pthread support on top of the
// SGI Challenge boxes of the era which featured up to sixteen CPU cards on a
// single bus with a special dedicated hardware bus for inter-CPU locking etc.
//
// These days Irix must have a standard posix-threads implementation, so this
// file should be obsoleted by   src/c/pthread/pthread-on-posix-threads.c
// and should probably be deleted by and by.  For the moment, I'm keeping it
// around for historical interest and comparison during debugging.
//
// Multicore (well, multi-processor) support for SGI Challenge machines (Irix 5.x)
// (no longer) implementing the API defined in the pthread section of
//
//     src/c/h/runtime-base.h
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

#include <stdio.h>
#include <string.h>

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
#include "runtime-globals.h"

// #define ARENA_FNAME  tmpnam(0)
#define ARENA_FNAME  "/tmp/sml-mp.mutex-arena"

#define INT_LIB7inc(n,i)  ((Val)TAGGED_INT_FROM_C_INT(TAGGED_INT_TO_C_INT(n) + (i)))
#define INT_LIB7dec(n,i)  (INT_LIB7inc(n,(-i)))

int   pth__done_pthread_create = FALSE;

static Mutex      AllocLock ();        
static Barrier*  AllocBarrier();

static usptr_t*	arena;								// Arena for shared sync chunks.

static ulock_t	MP_ArenaLock;							// Must be held to alloc/free a mutex.

static ulock_t	MP_ProcLock;							// Must be held to acquire/release procs.

Mutex	 pth__heapcleaner_mutex;					// Used only in   src/c/heapcleaner/pthread-heapcleaner-stuff.c

Mutex	 pth__heapcleaner_gen_mutex;					// Used only in   src/c/heapcleaner/make-strings-and-vectors-etc.c

Barrier* pth__heapcleaner_barrier;						// Used only with pth__wait_at_barrier prim, in   src/c/heapcleaner/pthread-heapcleaner-stuff.c

Mutex	 pth__timer_mutex;						// Apparently never used.



void   pth__start_up   () {
    // =============
    //
    // Called (only) from   src/c/main/runtime-main.c

    // set '_utrace = 1;' to debug shared arenas

    if (usconfig(CONF_LOCKTYPE, US_NODEBUG) == -1)   die ("usconfig failed in pth__start_up");

    usconfig(CONF_AUTOGROW, 0);

    if (usconfig(CONF_INITSIZE, 65536) == -1) 	die ("usconfig failed in pth__start_up");

    if ((arena = usinit(ARENA_FNAME)) == NULL) 	die ("usinit failed in pth__start_up");

    MP_ArenaLock			= AllocLock ();
    MP_ProcLock				= AllocLock ();
    pth__heapcleaner_mutex	= AllocLock ();
    pth__heapcleaner_gen_mutex	= AllocLock ();
    pth__timer_mutex		= AllocLock ();
    pth__heapcleaner_barrier	= AllocBarrier();
    //
    ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL, TAGGED_INT_FROM_C_INT(1) );
}



Pid   pth__get_pthread_id   ()   {
    //==================
    //
    return getpid ();
}

Pthread*  pth__get_pthread   ()   {
    //    ===============
    //
    // Return Pthread* for currently running pthread -- this
    // is needed to find record for current pthread in contexts
    // like signal handlers where it is not (otherwise) available.
    //    
    //
#if !NEED_PTHREAD_SUPPORT
   //
   return pthread_table__global[ 0 ];
#else
    int pid =  pth__get_pthread_id ();							// Since this just calls getpid(), the result is available in all contexts.  (That we care about. :-)
    //
    for (int i = 0;  i < MAX_PTHREADS;  ++i) {
	//
	if (pthread_table__global[i]->pid == pid)   return &pthread_table__global[ i ];	// pthread_table__global		def in   src/c/main/runtime-state.c
    }											// pthread_table__global exported via     src/c/h/runtime-base.h
    die "pth__get_pthread:  pid %d not found in pthread_table__global?!", pid;
#endif
}

static Mutex   allocate_mutex   ()   {
    //        =============
    //
    // Allocate and initialize a system mutex.

    ulock_t	mutex;

    if ((mutex = usnewlock(arena)) == NULL)   die ("allocate_mutex: cannot get mutex with usnewlock\n");

    usinitlock(mutex);
    usunsetlock(mutex);

    return mutex;
}
 

void   pth__mutex_lock   (Mutex mutex)   {
    // ===============
    //
    ussetlock(mutex);
}


void   pth__mutex_unlock   (Mutex mutex)   {
    // ===============
    //
    usunsetlock(mutex);
}


Bool   pth__mutex_maybe_lock   (Mutex mutex)   {
    // ===========
    //
    return ((Bool) uscsetlock(mutex, 1));		// Try once.
}


Mutex   pth__make_mutex   ()   {
    // ============
    //
    ulock_t mutex;

    ussetlock(   MP_ArenaLock );
        //
	mutex = allocate_mutex ();
        //
    usunsetlock( MP_ArenaLock );

    return mutex;
}



void   pth__free_mutex   (Mutex mutex)   {
    // ============
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
  


Barrier*   pth__make_barrier   ()   {
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



void   pth__free_barrier   (Barrier* barrierp)   {
    // ===============
    //
    ussetlock(MP_ArenaLock);
	//
	free_barrier( ebarrierp );
	//
    usunsetlock(MP_ArenaLock);
}



void   pth__wait_at_barrier   (Barrier* barrierp,  unsigned n)   {
    // ==========
    //
    barrier( barrierp, n );
}



void   pth__clear_barrier   (Barrier* barrierp)   {
    // ================
    //
    init_barrier(barrierp);
}


static void   fix_pnum   (int n)   {
    //        ========
    //
    // Dummy for now.
}
 


int   pth__max_pthreads   ()   {						// This fn gets exported to the Mythryl level; not used at the C level.
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
	PTHREAD_LOG_IF ("[waiting for self]\n");
	//
	continue;
    }

    PTHREAD_LOG_IF ("[new proc main: releasing mutex]\n");

    pth__mutex_unlock( MP_ProcLock );			// Implicitly handed to us by the parent.
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



Val   pth__pthread_create   (Task* task, Val arg)   {
    //====================
    //
    pth__done_pthread_create = TRUE;

    Task* p;
    Pthread* pthread;

    Val thread_arg  =  GET_TUPLE_SLOT_AS_VAL( arg, 0 );			// This is stored into   pthread->task->current_thread.
    Val closure_arg =  GET_TUPLE_SLOT_AS_VAL( arg, 1 );			// This is stored into   pthread->task->current_closure
									// and also              pthread->task->link.
    int i;

    PTHREAD_LOG_IF ("[acquiring proc]\n");

    pth__mutex_lock(MP_ProcLock);

    // Search for a suspended kernel thread to reuse:
    //
    for (i = 0;
	(i < MAX_PTHREADS)  &&  (pthread_table__global[i]->status != PTHREAD_IS_SUSPENDED);
	i++
    ) {
	continue;
    }

    PTHREAD_LOG_IF ("[checking for suspended processor]\n");

    if (i == MAX_PTHREADS) {
        //
	if (DEREF( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL ) == TAGGED_INT_FROM_C_INT( MAX_PTHREADS )) {
	    //
	    pth__mutex_unlock( MP_ProcLock );
	    say_error("[processors maxed]\n");
	    return HEAP_FALSE;
	}

	PTHREAD_LOG_IF ("[checking for NO_PROC]\n");

	// Search for a slot in which to put a new pthread:
	//
	for (i = 0;
	    (i < MAX_PTHREADS)  &&  (pthread_table__global[i]->status != IS_VOID);
	    i++
	){
	    continue;
	}

	if (i == MAX_PTHREADS) {
	    //
	    pth__mutex_unlock(MP_ProcLock);
	    say_error("[no processor to allocate]\n");
	    return HEAP_FALSE;
	}
    }

    PTHREAD_LOG_IF ("[using processor at index %d]\n", i);

    // Use pthread at index i:
    //
    pthread =  pthread_table__global[ i ];

    p =  pthread->task;

    p->exception_fate	=  PTR_CAST( Val,  handle_v + 1 );
    p->argument		=  HEAP_VOID;
    p->fate		=  PTR_CAST( Val, return_c);
    p->current_closure	=  closure_arg;
    p->program_counter	= 
    p->link_register	=  GET_CODE_ADDRESS_FROM_CLOSURE( closure_arg );
    p->current_thread	=  thread_arg;
  
    if (pthread->mode == IS_VOID) {
	//
        // Assume we get one:

	ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL, INT_LIB7inc( DEREF(ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL), 1) );

	if ((pthread->pid = make_pthread(p)) != -1) {
	    //
	    PTHREAD_LOG_IF ("[got a processor]\n");

	    pthread->mode = IS_RUNNING;

	    // make_pthread will release MP_ProcLock.

	    return HEAP_TRUE;

	} else {

	    ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL, INT_LIB7dec(DEREF(ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL), 1) );
	    pth__mutex_unlock(MP_ProcLock);
	    return HEAP_FALSE;
	}      

    } else {

	pthread->mode = IS_RUNNING;

	PTHREAD_LOG_IF ("[reusing a processor]\n");

	pth__mutex_unlock(MP_ProcLock);

	return HEAP_TRUE;
    }
}						// fun pth__pthread_create



void   pth__pthread_exit   (Task* task)   {
    // ==================
    //
    PTHREAD_LOG_IF ("[release_pthread: suspending]\n");

    call_heapcleaner( task, 1 );							// call_heapcleaner		def in   /src/c/heapcleaner/call-heapcleaner.c

    pth__mutex_lock(MP_ProcLock);

    task->pthread->mode = IS_BLOCKED;

    pth__mutex_unlock(MP_ProcLock);

    while (task->pthread->mode == IS_BLOCKED) {
	//
	call_heapcleaner( task, 1 );										// Need to be continually available for garbage collection.
    }

    PTHREAD_LOG_IF ("[release_pthread: resuming]\n");

    run_mythryl_task_and_runtime_eventloop( task );								// run_mythryl_task_and_runtime_eventloop		def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c

    die ("return after run_mythryl_task_and_runtime_eventloop(task) in mp_release_pthread\n");
}



int   pth__get_active_pthread_count   ()   {
    //============================
    //
    int ap;

    pth__mutex_lock(MP_ProcLock);
        ap = TAGGED_INT_TO_C_INT( DEREF(ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL) );
    pth__mutex_unlock(MP_ProcLock);

    return ap;
}



void   pth__shut_down   ()   {
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


