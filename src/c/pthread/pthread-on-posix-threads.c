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
#include <errno.h>
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

// https://computing.llnl.gov/tutorials/pthreads/man/sched_setscheduler.txt
// https://computing.llnl.gov/tutorials/pthreads/#ConditionVariables
// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_init.html
// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_wait.html
// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_signal.html
// pthread_cond_t myconvar = PTHREAD_COND_INITIALIZER;

// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cancel.html
// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_getconcurrency.html
// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_timedlock.html
// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_equal.html

// http://publib.boulder.ibm.com/infocenter/pseries/v5r3/index.jsp?topic=/com.ibm.aix.genprogc/doc/genprogc/rwlocks.htm

// In /usr/include/bits/local_lim.h PTHREAD_THREADS = 1024.



// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_init.html
// #include <pthread.h>
//
// int pthread_cond_destroy(pthread_cond_t *cond);
// int pthread_cond_init(pthread_cond_t *restrict cond, const pthread_condattr_t *restrict attr);
// pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 

// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_wait.html
// #include <pthread.h>
//
// int pthread_cond_timedwait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex, const struct timespec *restrict abstime);
// int pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex); 

// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_signal.html
// #include <pthread.h>
//
// int pthread_cond_broadcast(pthread_cond_t *cond);
// int pthread_cond_signal(pthread_cond_t *cond); 

// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_timedlock.html
//
// #include <pthread.h>
// #include <time.h>
//
// int pthread_mutex_timedlock( pthread_mutex_t *restrict mutex, const struct timespec *restrict abs_timeout );


int   pth__done_pthread_create__global = FALSE;
    //
    // This boolean flag starts out FALSE and is set TRUE
    // the first time   pth__pthread_create   is called.
    //
    // We can use simple mutex-free monothread-style logic
    // in the heapcleaner (etc) so long as this is FALSE,
    // per the Fairness Principle (processes that do not
    // use something should not have to pay for it).


// Some statically allocated locks.
//
// We try to put each mutex in its own cache line
// to prevent cores thrashing against each other
// trying to get control of logically unrelated mutexs:
//
// It would presumably be good to force cache-line-size
// alignment here, but I don't know how, short of
// malloc'ing and checking alignment at runtime:
/**/												char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];
       Mutex	 pth__heapcleaner_mutex__global		= PTHREAD_MUTEX_INITIALIZER;		char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];		// Used only in   src/c/heapcleaner/pthread-heapcleaner-stuff.c
       Mutex	 pth__heapcleaner_gen_mutex__global	= PTHREAD_MUTEX_INITIALIZER;		char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];		// Used only in   src/c/heapcleaner/make-strings-and-vectors-etc.c
       Mutex	 pth__timer_mutex__global		= PTHREAD_MUTEX_INITIALIZER;		char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];		// Apparently never used.
static Mutex	      proc_mutex__local			= PTHREAD_MUTEX_INITIALIZER;		char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];		// Apparently never used.

       Condvar	 pth__unused_condvar__global		= PTHREAD_COND_INITIALIZER;		char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];		// Never used.



Barrier  pth__heapcleaner_barrier__global;					// Used only with pth__wait_at_barrier prim, in   src/c/heapcleaner/pthread-heapcleaner-stuff.c


// Some placeholder fns just so I can start
// getting other files -- in particular   src/c/heapcleaner/pthread-heapcleaner-stuff.c
// -- to compile:
//
Val      pth__pthread_create		(Task* task, Val thread, Val closure)			{ die("pth__pthread_create() not implemented yet"); return (Val)NULL;}
    //   ===================
    //
    // Called (only) by   make_pthread()   in   src/c/lib/pthread/libmythryl-pthread.c

void     pth__pthread_exit		(Task* task)				{ die("pth__pthread_exit() not implemented yet"); }
    //   =================
    //
    // Called (only) by   release_pthread()   in   src/c/lib/pthread/libmythryl-pthread.c

// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_create.html
// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_exit.html


// pthread_barrier_init(&barr, NULL, THREADS)	// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_barrier_init.html
//     int rc = pthread_barrier_wait(&barr);    // http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_barrier_wait.html
// if(rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD)
//   {
//    printf("Could not wait on barrier\n");
//    exit(-1);
//   }


void   pth__start_up   (void)   {
    // =============
    //
    // Start-of-the-world initialization stuff.
    // We get called very early by   do_start_of_world_stuff   in   src/c/main/runtime-main.c
    //
    // We could allocate our static global mutexes here
    // if necessary, but we don't need to because the
    // posix-threads API allows us to just statically
    // initialize them to PTHREAD_MUTEX_INITIALIZER.

    //
    ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL, TAGGED_INT_FROM_C_INT(1) );

}

void   pth__shut_down (void) {
    // ==============
    //
    // Our present implementation need do nothing at end-of-world shutdown.
    //
    // We get called from   do_end_of_world_stuff_and_exit()   in   src/c/main/runtime-main.c
    // and also             die()  and  assert_fail()          in   src/c/main/error.c
}

// NB: All the error returns in this file should interpret the error number; I forget the syntax offhand. XXX SUCKO FIXME -- 2011-11-03 CrT

char*    pth__mutex_init   (Mutex* mutex) {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_init.html
    //   ===============
    //
    if (pthread_mutex_init( mutex, NULL ))	return "pth__mutex_init: Unable to initialize mutex.";
    else					return NULL;
}

char*    pth__mutex_destroy   (Mutex* mutex)   {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_init.html
    //   ==================
    //
    if (pthread_mutex_destroy( mutex ))		return "pth__mutex_destroy: Unable to destroy mutex";
    else					return NULL;
}

void   pth__mutex_lock  (Mutex* mutex) {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_lock.html
    // ===============
    //
    if (!pth__done_pthread_create__global)   return;
    //
    if (pthread_mutex_lock( mutex )) {
	//
	die("pth__mutex_lock: Unable to acquire lock.");
    }
}

Bool   pth__mutex_trylock   (Mutex* mutex)   {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_lock.html
    // ==================
    //
    int err =  pthread_mutex_trylock( mutex );
    //
    switch (err) {
	//
	case 0: 	return FALSE;						// Successfully acquired lock.
	case EBUSY:	return TRUE;						// Lock was already taken.
	//
	default:
	    die("pth__mutex_trylock: Error while attempting to test lock.");
    }
}

void   pth__mutex_unlock   (Mutex* mutex) {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_lock.html
    // =================
    //
    if (!pth__done_pthread_create__global) return;
    //
    if (pthread_mutex_unlock( mutex )) {
	//
	die("pth__mutex_unlock: Unable to release lock.");
    }
}


void     pth__condvar_init ( Condvar* condvar ) {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_init.html
    //   =================
    //
    if (pthread_cond_init( condvar, NULL )) {
	//
	die("pth__condvar_init: Unable to initialize condition variable.");
    }
}

void   pth__condvar_destroy   (Condvar* condvar) {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_init.html
    // ====================
    //
    if (pthread_cond_destroy( condvar )) {
	//
	die("pth__condvar_destroy: Unable to destroy condition variable.");
    }
}

void   pth__condvar_wait   (Condvar* condvar, Mutex* mutex) {			// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_wait.html
    // =================
    //
    if (pthread_cond_wait( condvar, mutex )) {
	//
	die("pth__condvar_wait: Unable to wait on condition variable.");
    }	
}

void   pth__condvar_signal   (Condvar* condvar) {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_signal.html
    // ===================
    //
    if (pthread_cond_signal( condvar )) {
	//
	die("pth__condvar_signal: Unable to signal on condition variable.");
    }	
}

void   pth__condvar_broadcast   (Condvar* condvar) {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_signal.html
    // ======================
    //
    if (pthread_cond_broadcast( condvar )) {
	//
	die("pth__condvar_broadcast: Unable to broadcast on condition variable.");
    }	
}


void   pth__barrier_init   (Barrier* barrier, int threads) {			// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_barrier_init.html
    // =================
    //
    if (pthread_barrier_init( barrier, NULL, (unsigned) threads)) {
	//
	die("pth__barrier_init: Unable to initialize barrier.");
    }
}

void   pth__barrier_destroy   (Barrier* barrier) {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_barrier_init.html
    // ====================
    //
    if (pthread_barrier_destroy( barrier )) {
	//
	die("pth__barrier_init: Unable to destroy barrier.");
    }
}


Bool   pth__barrier_wait   (Barrier* barrier) {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_barrier_wait.html
    // =================
    //
    int err =  pthread_barrier_wait( barrier );
    //
    switch (err) {
	//
	case PTHREAD_BARRIER_SERIAL_THREAD:	return 1;			// Exactly one pthread gets this return value when released from barrier.
	case 0:					return 0;			// All other threads at barrier get this.
	//
	default:
	    die("pth__barrier_wait: Fatal error while blocked at barrier.");
    }
}



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
    //    ================
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


int   pth__get_active_pthread_count   ()   {
    //=============================
    //
    // This function is currently called (only) in
    // the critical spinloop in
    //
    //     src/c/heapcleaner/pthread-heapcleaner-stuff.c
    //
    // while we're waiting for all pthreads to
    // enter heapcleaning mode.
  
    pth__mutex_lock( &proc_mutex__local );						// What could go wrong here if we didn't use a mutex...?
	//										// (Seems like reading a refcell is basically atomic anyhow.)
        int active_pthread_count = TAGGED_INT_TO_C_INT( DEREF(ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL) );
	//
    pth__mutex_unlock ( &proc_mutex__local );

    return  active_pthread_count;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  SPINLOCK CAUTION
//
// http://stackoverflow.com/questions/6603404/when-is-pthread-spin-lock-the-right-thing-to-use-over-e-g-a-pthread-mutex
//
// Q: Given that pthread_spin_lock is available, when would I use it,
//    and when should one not use them?  I.e. how would I decide to
//    protect some shared data structure with either a pthread mutex
//    or a pthread spinlock ?
//
// A: The short answer is that a spinlock can be better when you plan
//    to hold the lock for an extremely short interval (for example
//    to do nothing but increment a counter), and contention is expected
//    to be rare, but the operation is occurring often enough to be a
//    potential performance bottleneck.
//
//    The advantages of a spinlock over a mutex are:
//
//    1 On unlock, there is no need to check if other threads may be waiting
//      for the lock and waking them up. Unlocking is simply a single atomic write instruction.
//
//    2 Failure to immediately obtain the lock does not put your thread
//      to sleep, so it may be able to obtain the lock with much lower
//      latency as soon a it does become available.
//
//    3 There is no risk of cache pollution from entering kernelspace
//      to sleep or wake other threads.
//
//   Point 1 will always stand, but points 2 and 3 are of somewhat
//   diminished usefulness if you consider that good mutex implementations
//   will probably spin a decent number of times before asking the kernel
//   for help waiting.
//
//   Now, the long answer:
//
//   What you need to ask yourself before using spinlocks is whether these
//   potential advantages outweigh one rare but very real disadvantage:
//
//       What happens when the thread that holds the lock gets
//      interrupted by the scheduler before it can release the lock.
//
//   This is of course rare, but it can happen even if the lock is just held
//   for a single variable-increment operation or something else equally trivial.
//
//   In this case, any other threads attempting to obtain the lock will
//   keep spinning until the thread the holds the lock gets scheduled and
//   has a chance to release the lock.
//
//   This might NEVER happen if the threads trying to obtain the lock
//   have higher priorities than the thread that holds the lock!
//
//   That may be an extreme case, but even without different priorities
//   in play, there can be very long delays before the lock owner gets
//   scheduled again, and worst of all, once this situation begins, it
//   can quickly escalate as many threads, all hoping to get the lock,
//   begin spinning on it, tying up more processor time, and further
//   delaying the scheduling of the thread that could release the lock.
//
//   As such, I would be careful with spinlocks... :-)
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////



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
    //         ==============
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
    // =================
    //
    usunsetlock(mutex);
}


Bool   pth__mutex_maybe_lock   (Mutex mutex)   {
    // =====================
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



static void   fix_pnum   (int n)   {
    //        ========
    //
    // Dummy for now.
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

									// typedef   struct task   Task;	def in   src/c/h/runtime-base.h
									// struct task				def in   src/c/h/task.h
Val   pth__pthread_create   (Task* task, Val current_thread, Val closure_arg)   {
    //===================
    //
    // This fn is called (only) by   make_pthread ()   in   src/c/lib/pthread/libmythryl-pthread.c
    //
    pth__done_pthread_create__global = TRUE;

    Task*    task;
    Pthread* pthread;

    int i;

    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say("[acquiring proc]\n");
    #endif

    pth__mutex_lock( MP_ProcLock );

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
	    pth__mutex_unlock( MP_ProcLock );
	    say_error("[processors maxed]\n");
	    return HEAP_FALSE;
	}
	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say("[checking for NO_PROC]\n");
	#endif

	// Search for a slot in which to put a new pthread
	//
	for (i = 0;
	    (i < MAX_PTHREADS)  &&  (pthread_table__global[i]->status != NO_PTHREAD_ALLOCATED);
	    i++
	){
	    continue;
	}

	if (i == MAX_PTHREADS) {
	    //
	    pth__mutex_unlock( MP_ProcLock );
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

    task =  pthread->task;

    task->exception_fate	=  PTR_CAST( Val,  handle_v + 1 );
    task->argument		=  HEAP_VOID;
    //
    task->fate			=  PTR_CAST( Val, return_c);
    task->current_closure	=  closure_arg;
    //
    task->program_counter	= 
    task->link_register		=  GET_CODE_ADDRESS_FROM_CLOSURE( closure_arg );
    //
    task->current_thread	=  current_thread;
  
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
	    pth__mutex_unlock(MP_ProcLock);
	    return HEAP_FALSE;
	}      

    } else {

	pthread->status = PTHREAD_IS_RUNNING;

	#ifdef NEED_PTHREAD_SUPPORT_DEBUG
	    debug_say ("[reusing a processor]\n");
	#endif

	pth__mutex_unlock(MP_ProcLock);

	return HEAP_TRUE;
    }
}						// fun pth__pthread_create



void   pth__pthread_exit   (Task* task)   {
    // ====================
    //
    #ifdef NEED_PTHREAD_SUPPORT_DEBUG
	debug_say("[release_pthread: suspending]\n");
    #endif

    call_heapcleaner( task, 1 );							// call_heapcleaner		def in   /src/c/heapcleaner/call-heapcleaner.c

    pth__mutex_lock(MP_ProcLock);

    task->pthread->status = PTHREAD_IS_SUSPENDED;

    pth__mutex_unlock(MP_ProcLock);

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


