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

#include <stdio.h>
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



int   pth__done_acquire_pthread__global = FALSE;
    //
    // This boolean flag starts out FALSE and is set TRUE
    // the first time   pth__acquire_pthread   is called.
    //
    // We can use simple mutex-free monothread logic in
    // the heapcleaner (etc) so long as this is FALSE.


// Some statatically allocated locks.
//
// We try to put each mutex in its own cache line
// to prevent cores thrashing against each other
// trying to get control of logically unrelated mutexs:
//
// It would presumably be good to force cache-line-size
// alignment here, but I don't know how, short of
// malloc'ing and checking alignment at runtime:
/**/											char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];
Mutex	 pth__heapcleaner_mutex__global		= PTHREAD_MUTEX_INITIALIZER;		char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];		// Used only in   src/c/heapcleaner/pthread-heapcleaner-stuff.c
Mutex	 pth__heapcleaner_gen_mutex__global	= PTHREAD_MUTEX_INITIALIZER;		char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];		// Used only in   src/c/heapcleaner/make-strings-and-vectors-etc.c
Mutex	 pth__timer_mutex__global		= PTHREAD_MUTEX_INITIALIZER;		char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];		// Apparently never used.



Barrier* pth__heapcleaner_barrier__global;	;					// Used only with pth__wait_at_barrier prim, in   src/c/heapcleaner/pthread-heapcleaner-stuff.c


// Some placeholder fns just so I can start
// getting other files -- in particular   src/c/heapcleaner/pthread-heapcleaner-stuff.c
// -- to compile:
//
void     pth__shut_down			()					{}
Mutex    pth__make_mutex		()					{ die("pth__make_mutex() not implemented yet");  }
void     pth__free_mutex		(Mutex mutex)				{ die("pth__free_mutex() not implemented yet"); }
Barrier* pth__make_barrier		()					{ die("pth__make_barrier() not implemented yet"); return (Barrier*)NULL; }
void     pth__free_barrier		(Barrier* barrierp)			{ die("pth__free_barrier() not implemented yet"); }
void     pth__wait_at_barrier		(Barrier* barrierp,  unsigned n)	{ die("pth__wait_at_barrier() not implemented yet"); }
void     pth__clear_barrier		(Barrier* barrierp)			{ die("pth__clear_barrier() not implemented yet"); }
int      pth__max_pthreads		()					{ die("pth__max_pthreads() not implemented yet"); return 0; }	// Why not just use MAX_PTHEADS?  Myabe: Because MAX_PTHREADS should not exist -- should be dynamically expandable?
Val      pth__acquire_pthread		(Task* task, Val arg)			{ die("pth__acquire_pthread() not implemented yet"); return (Val)NULL;}
void     pth__release_pthread		(Task* task)				{ die("pth__release_pthread() not implemented yet"); }
int      pth__get_active_pthread_count	()					{ die("pth__get_active_pthread_count() not implemented yet"); return 0; }


void     pth__start_up   (void)   {
    //
    // Start-of-the-world initialization stuff.
    // We get called very early by   do_start_of_world_stuff   in   src/c/main/runtime-main.c
    //
    // We could allocate our static global mutexes here
    // if necessary, but we don't need to because the
    // posix-threads API allows us to just statically
    // initialize them to PTHREAD_MUTEX_INITIALIZER.

//   pth__heapcleaner_barrier__global	= AllocBarrier();
    //
    ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL, TAGGED_INT_FROM_C_INT(1) );

}

void     pth__acquire_mutex		(Mutex* mutex)				{ if (!pth__done_acquire_pthread__global) return;   die("pth__acquire_mutex() not implemented yet"); }
void     pth__release_mutex		(Mutex* mutex)				{ if (!pth__done_acquire_pthread__global) return;   die("pth__release_mutex() not implemented yet"); }
// pthread_mutex_lock(   &mutex1 );
// pthread_mutex_unlock( &mutex1 );



Pid   pth__get_pthread_id   ()   {
    //===================
    //
    // Return a unique small-int id distinguishing
    // the currently running pthread from all other
    // pthreads.  On posix-threads we can just use
    // getpid() for this.  On some older thread packages
    // we had to do other stuff, and we might possibly
    // have to do so in future on some non-posix-threads
    // implementation, so we maintain the abstraction here:
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
    }											// pthread_table__global exported via     src/c/h/runtime-pthread.h
    die "pth__get_pthread:  pid %d not found in pthread_table__global?!", pid;
#endif
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




void   pth__start_up   () {
    // =============
    //
    // Called (only) from   src/c/main/runtime-main.c

    // set '_utrace = 1;' to debug shared arenas

    if (usconfig(CONF_LOCKTYPE, US_NODEBUG) == -1)   die ("usconfig failed in pth__start_up");

    usconfig(CONF_AUTOGROW, 0);

    if (usconfig(CONF_INITSIZE, 65536) == -1) 	die ("usconfig failed in pth__start_up");

    if ((arena = usinit(ARENA_FNAME)) == NULL) 	die ("usinit failed in pth__start_up");

    MP_ArenaLock		= AllocLock ();
    MP_ProcLock			= AllocLock ();
    pth__heapcleaner_mutex__global	= AllocLock ();
    pth__heapcleaner_gen_mutex__global	= AllocLock ();
    pth__timer_mutex__global	= AllocLock ();
    pth__heapcleaner_barrier__global	= AllocBarrier();
    //
    ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL, TAGGED_INT_FROM_C_INT(1) );
}

void   pth__shut_down   ()   {
    // ==============
    //
    usdetach( arena );												// 'usdetach' appears nowhere else in codebase; must be the SGI equivalent to posix 'munmap'
}


Pid   pth__pthread_id   ()   {
    //===============
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
 

void   pth__acquire_mutex   (Mutex mutex)   {
    // =================
    //
    ussetlock(mutex);
}


void   pth__release_mutex   (Mutex mutex)   {
    // ==================
    //
    usunsetlock(mutex);
}


Bool   pth__maybe_acquire_mutex   (Mutex mutex)   {
    // ========================
    //
    return ((Bool) uscsetlock(mutex, 1));		// Try once.
}


Mutex   pth__make_mutex   ()   {
    //  ===============
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
    // ===============
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
    //     =================
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
    // =================
    //
    ussetlock(MP_ArenaLock);
	//
	free_barrier( ebarrierp );
	//
    usunsetlock(MP_ArenaLock);
}



void   pth__wait_at_barrier   (Barrier* barrierp,  unsigned n)   {
    // ====================
    //
    barrier( barrierp, n );
}



void   pth__clear_barrier   (Barrier* barrierp)   {
    // ==================
    //
    init_barrier(barrierp);
}


static void   fix_pnum   (int n)   {
    //        ========
    //
    // Dummy for now.
}
 


int   pth__max_pthreads   ()   {						// This fn gets exported to the Mythryl level; not used at the C level.
    //=================
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

    pth__release_mutex( MP_ProcLock );			// Implicitly handed to us by the parent.
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



Val   pth__acquire_pthread   (Task* task, Val arg)   {
    //====================
    //
    pth__done_acquire_pthread__global = TRUE;

    Task* p;
    Pthread* pthread;

    Val thread_arg  =  GET_TUPLE_SLOT_AS_VAL( arg, 0 );			// This is stored into   pthread->task->thread.
    Val closure_arg =  GET_TUPLE_SLOT_AS_VAL( arg, 1 );			// This is stored into   pthread->task->closure
									// and also              pthread->task->link.
    int i;

    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say("[acquiring proc]\n");
    #endif

    pth__acquire_mutex(MP_ProcLock);

    // Search for a suspended kernel thread to reuse:
    //
    for (i = 0;
	(i < MAX_PTHREADS)  &&  (pthread_table__global[i]->status != PTHREAD_IS_SUSPENDED);
	i++
    ) {
	continue;
    }
    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say("[checking for suspended processor]\n");
    #endif

    if (i == MAX_PTHREADS) {
        //
	if (DEREF( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL ) == TAGGED_INT_FROM_C_INT( MAX_PTHREADS )) {
	    //
	    pth__release_mutex( MP_ProcLock );
	    say_error("[processors maxed]\n");
	    return HEAP_FALSE;
	}
	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say("[checking for NO_PROC]\n");
	#endif

	// Search for a slot in which to put a new proc
	//
	for (i = 0;
	    (i < MAX_PTHREADS)  &&  (pthread_table__global[i]->status != NO_PTHREAD_ALLOCATED);
	    i++
	){
	    continue;
	}

	if (i == MAX_PTHREADS) {
	    //
	    pth__release_mutex( MP_ProcLock );
	    say_error("[no processor to allocate]\n");
	    return HEAP_FALSE;
	}
    }
    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say("[using processor at index %d]\n", i);
    #endif

    // Use pthread at index i:
    //
    pthread =  pthread_table__global[ i ];

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

	ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL, INT_LIB7inc( DEREF(ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL), 1) );

	if ((pthread->pid = make_pthread(p)) != -1) {
	    //
	    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
		debug_say ("[got a processor]\n");
	    #endif

	    pthread->status = PTHREAD_IS_RUNNING;

	    // make_pthread will release MP_ProcLock.

	    return HEAP_TRUE;

	} else {

	    ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL, INT_LIB7dec(DEREF(ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL), 1) );
	    pth__release_mutex(MP_ProcLock);
	    return HEAP_FALSE;
	}      

    } else {

	pthread->status = PTHREAD_IS_RUNNING;

	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("[reusing a processor]\n");
	#endif

	pth__release_mutex(MP_ProcLock);

	return HEAP_TRUE;
    }
}						// fun pth__acquire_pthread



void   pth__release_pthread   (Task* task)   {
    // ====================
    //
    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say("[release_pthread: suspending]\n");
    #endif

    call_heapcleaner( task, 1 );							// call_heapcleaner		def in   /src/c/heapcleaner/call-heapcleaner.c

    pth__acquire_mutex(MP_ProcLock);

    task->pthread->status = PTHREAD_IS_SUSPENDED;

    pth__release_mutex(MP_ProcLock);

    while (task->pthread->status == PTHREAD_IS_SUSPENDED) {
	//
	call_heapcleaner( task, 1 );										// Need to be continually available for garbage collection.
    }
    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say("[release_pthread: resuming]\n");
    #endif

    run_mythryl_task_and_runtime_eventloop( task );								// run_mythryl_task_and_runtime_eventloop		def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c

    die ("return after run_mythryl_task_and_runtime_eventloop(task) in mp_release_pthread\n");
}



int   pth__get_active_pthread_count   ()   {
    //=============================
    //
    int ap;

    pth__acquire_mutex(MP_ProcLock);

        ap = TAGGED_INT_TO_C_INT( DEREF(ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL) );

    pth__release_mutex(MP_ProcLock);

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


