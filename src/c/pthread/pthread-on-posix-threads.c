// pthread-on-posix-threads.c
//
// For background see the "Overview" comments in:
//
//     src/lib/std/src/pthread.api
//
//
// This file contains our actual calls directly
// to the <pthead.h> routines.
//
// This code is derived in (small!) part from the original
// 1994 sgi-mp.c file from the SML/NJ codebase.



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
#include "heapcleaner.h"


#define INCREASE_BY(n,i)  ((Val)TAGGED_INT_FROM_C_INT(TAGGED_INT_TO_C_INT(n) + (i)))
#define DECREASE_BY(n,i)  (INCREASE_BY(n,(-i)))


//
int   pth__done_pthread_create__global  =  FALSE;
    //================================
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
static Mutex	      pthread_table_mutex__local	= PTHREAD_MUTEX_INITIALIZER;		char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];		// Used in this file to serialize access to pthread_table__global[].

       Condvar	 pth__unused_condvar__global		= PTHREAD_COND_INITIALIZER;		char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];		// Never used.


//
Barrier  pth__heapcleaner_barrier__global;					// Used only with pth__wait_at_barrier prim, in   src/c/heapcleaner/pthread-heapcleaner-stuff.c





//
static void*  pthread_main   (void* task_as_voidptr)   {
    //        ============
    //
    // This is the top-level function we execute within
    // pthreads spawned from within Mythryl code:
    // pth__pthread_create() passes us to  pthread_create().
    //
    // Should we maybe be clearing some or all of the signal mask here?
    //
    //     "The signal state of the new thread shall be initialized as follows:
    //          The signal mask shall be inherited from the creating thread.
    //          The set of signals pending for the new thread shall be empty."
    //
    //      -- http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_create.html	


    Task* task = (Task*) task_as_voidptr;					// The <pthread.h> API for pthread_create requires that our arg be cast to void*; cast it back to its real type.


    pth__mutex_unlock( &pthread_table_mutex__local );				// This lock was acquired by pth__pthread_create (below).

    run_mythryl_task_and_runtime_eventloop( task );				// run_mythryl_task_and_runtime_eventloop		def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c


    // run_mythryl_task_and_runtime_eventloop should never return:
    //
    die ("pthread_main:  Returned fromd run_mythryl_task_and_runtime_eventloop()?!\n");

    return  (void*) NULL;							// Cannot execute; only to keep gcc quiet.
}




										// typedef   struct task   Task;	def in   src/c/h/runtime-base.h
										// struct task				def in   src/c/h/task.h

char* pth__pthread_create   (int* pthread_table_slot, Val current_thread, Val closure_arg)   {
    //===================
    //
    // Run 'closure_arg' in its own kernel thread.
    //
    // This fn is called (only) by   spawn_pthread ()   in   src/c/lib/pthread/libmythryl-pthread.c
    //
    pth__done_pthread_create__global = TRUE;					// Once set TRUE, this is never set back to FALSE. (So locking is not an issue.)

    Task*    task;
    Pthread* pthread;
    int      i;

    PTHREAD_LOG_IF ("[Searching for free pthread]\n");

    pth__mutex_lock( &pthread_table_mutex__local );				// Always first step before reading/writing pthread_table__global.
										// We don't use the PTH__MUTEX_LOCK macro because at this point
										// we know pth__done_pthread_create__global == TRUE.
    //
    if (DEREF( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL ) == TAGGED_INT_FROM_C_INT( MAX_PTHREADS )) {
	//
	pth__mutex_unlock( &pthread_table_mutex__local );
	return "pthread_table__global full -- increase MAX_PTHREADS";
    }

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
	pth__mutex_unlock( &pthread_table_mutex__local );
	return  "pthread_table__global full -- increase MAX_PTHREADS?";
    }

    PTHREAD_LOG_IF ("[using pthread_table__global slot %d]\n", i);

    // Use pthread at index i:
    //
    *pthread_table_slot = i;
    //
    pthread =  pthread_table__global[ i ];

    task =  pthread->task;

    task->exception_fate	=  PTR_CAST( Val,  handle_uncaught_exception_closure_v + 1 );	// Defined by   ASM_CLOSURE(handle_uncaught_exception_closure);   in   src/c/main/construct-runtime-package.c
												// in reference to handle_uncaught_exception_closure_asm	  in   src/c/machine-dependent/prim*asm 
    task->argument		=  HEAP_VOID;
    //
    task->fate			=  PTR_CAST( Val, return_to_c_level_c);				// Defined by   ASM_CONT(return_to_c_level);			  in   src/c/main/construct-runtime-package.c
												// in reference to return_to_c_level_asm            		  in   src/c/machine-dependent/prim*asm 
    task->current_closure	=  closure_arg;
    //
    task->program_counter	= 
    task->link_register		=  GET_CODE_ADDRESS_FROM_CLOSURE( closure_arg );
    //
    task->current_thread	=  current_thread;
  

    //
    // Optimistically increment active-pthreads count:
    //
    ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL,
	    INCREASE_BY( DEREF(ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL), 1)
    );

    pthread->status = PTHREAD_IS_RUNNING;						// Moved this above pthread_create() because that seems safer,
										    // otherwise child might run arbitrarily long without this being set. -- 2011-11-10 CrT
    int err =   pthread_create(
		    //
		    &task->pthread->pid,						// RESULT. NB: Passing a pointer directly to task->pthread->pid ensures that field is always
										    //         valid as seen by both parent and child threads, without using spinlocks or such.
										    //	   Passing the pointer is safe (only) because 'pid' is of type pthread_t from <pthread.h>
										    //	   -- we define field 'pid' as 'Pid' in src/c/h/pthread.h
										    //	   and   typedef pthread_t Pid;   in   src/c/h/runtime-base.h

		    NULL,								// Provision for attributes -- API futureproofing.

		    pthread_main,  (void*) task					// Function + argument to run in new kernel thread.
		);

    if (!err) {									// Successfully spawned new kernel thread.
	//
	return NULL;								// Report success. NB: Child thread (i.e., pthread_main() above)  will unlock  pthread_table_mutex__local  for us.

    } else {									// Failed to spawn new kernel thread.

	pthread->status = NO_PTHREAD_ALLOCATED;					// Note pthread record (still) has no associated kernel thread.

	ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL,				// Restore active-threads count to its original value
		DECREASE_BY(DEREF(ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL), 1)	// since our optimism proved unwarranted. 
	);

	pth__mutex_unlock( &pthread_table_mutex__local );

	switch (err) {
	    //
	    case EAGAIN:	return "pth__pthread_create: Insufficient resources to create posix thread: May have reached PTHREAD_THREADS_MAX.";
	    case EPERM:		return "pth__pthread_create: You lack permissions to set requested scheduling.";
	    case EINVAL:	return "pth__pthread_create: Invalid attributes.";
	    default:		return "pth__pthread_create: Undocumented error returned by pthread_create().";
	}
    }      
}						// fun pth__pthread_create



//
void   pth__pthread_exit   (Task* task)   {
    // =================
    //
    // Called (only) by   release_pthread()   in   src/c/lib/pthread/libmythryl-pthread.c
    //
    PTHREAD_LOG_IF ("[release_pthread: suspending]\n");

    call_heapcleaner( task, 1 );										// call_heapcleaner		def in   /src/c/heapcleaner/call-heapcleaner.c
	//
	// I presume this call must be intended to sweep all live
	// values from this thread's private generation-zero buffer
	// into the shared generation-1 buffer, so that nothing
	// will be lost if re-initializing the generation-zero
	// buffer for a new thread.   -- 2011-11-10 CrT


    pth__mutex_lock(    &pthread_table_mutex__local );								// I cannot honestly see what locking achieves here. -- 2011-11-10 CrT
	//													// We don't use the PTH__MUTEX_LOCK macro because at this point
	//													// we know pth__done_pthread_create__global == TRUE.
	task->pthread->status = NO_PTHREAD_ALLOCATED;
	//
    pth__mutex_unlock(  &pthread_table_mutex__local );

    pthread_exit( NULL );								// "The pthread_exit() function cannot return to its caller."   -- http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_exit.html
}



char*    pth__pthread_join   (Task* task_joining, int pthread_to_join) {		// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_join.html
    //   =================
    //
    // Called (only) by   join_pthread()   in   src/c/lib/pthread/libmythryl-pthread.c

    // 'pthread_to_join' should have been returned by
    // pth__pthread_create  (above) and should be an index into
    // pthread_table__global[], but let's sanity-check it: 
    //
    if (pthread_to_join < 0
    ||  pthread_to_join >= MAX_PTHREADS)    return "pth__pthread_join: Bogus value for pthread_to_join.";

    Pthread* pthread =  pthread_table__global[ pthread_to_join ];			// pthread_table__global	def in   src/c/main/runtime-state.c

    if (pthread->status != PTHREAD_IS_RUNNING)  {
	//
	return "pth__pthread_join: Bogus value for pthread-to-join (already-dead thread?)";
    }

    int     err =  pthread_join( pthread->pid, NULL );				// NULL is a void** arg that can return result of joined thread. We ignore it
    switch (err) {								// because the typing would be a pain: we'd have to return Exception, probably -- ick!
	//
	case 0:		return NULL;						// Success.
	case ESRCH:	return "pth__pthread_join: No such thread.";
	case EDEADLK:	return "pth__pthread_join: Attempt to join self (or other deadlock).";
	case EINVAL:	return "pth__pthread_join: Not a joinable thread.";
	default:	return "pth__pthread_join: Undocumented error.";
    }
}



//
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
//
void   pth__shut_down (void) {
    // ==============
    //
    // Our present implementation need do nothing at end-of-world shutdown.
    //
    // We get called from   do_end_of_world_stuff_and_exit()   in   src/c/main/runtime-main.c
    // and also             die()  and  assert_fail()          in   src/c/main/error-reporting.c
}

// NB: All the error returns in this file should interpret the error number; I forget the syntax offhand. XXX SUCKO FIXME -- 2011-11-03 CrT
//
char*    pth__mutex_init   (Mutex* mutex) {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_init.html
    //   ===============
    //
    int err =  pthread_mutex_init( mutex, NULL );
    //
    switch (err) {
	//
	case 0:				return NULL;				// Success.
	case ENOMEM:			return "pth__mutex_init: Insufficient ram to initialize mutex";
	case EAGAIN:			return "pth__mutex_init: Insufficient (non-ram) resources to initialize mutex";
	case EPERM:			return "pth__mutex_init: Caller lacks privilege to initialize mutex";
	case EBUSY:			return "pth__mutex_init: Attempt to reinitialize the object referenced by mutex, a previously initialized, but not yet destroyed, mutex.";
	case EINVAL:			return "pth__mutex_init: Invalid attribute";
	default:			return "pth__mutex_init: Undocumented error return from pthread_mutex_init()";
    }
}

//
char*    pth__mutex_destroy   (Mutex* mutex)   {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_init.html
    //   ==================
    //
    int err =  pthread_mutex_destroy( mutex );
    //
    switch (err) {
	//
	case 0:				return NULL;				// Success.
	case EBUSY:			return "pth__mutex_destroy: attempt to destroy the object referenced by mutex while it is locked or referenced (eg, while being used in a pthread_cond_timedwait() or pthread_cond_wait()) by another thread.";
	case EINVAL:			return "pth__mutex_destroy: invalid mutex.";
	default:			return "pth__mutex_destroy: Undocumented error return from pthread_mutex_destroy()";
    }
}
//
char*  pth__mutex_lock  (Mutex* mutex) {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_lock.html
    // ===============
    //
    //
    int err = pthread_mutex_lock( mutex );
    //
    switch (err) {
	//
	case 0:				return NULL;				// Success.
	case EINVAL:			return "pth__mutex_lock: Invalid mutex or mutex has PTHREAD_PRIO_PROTECT set and calling thread's priority is higher than mutex's priority ceiling.";
	case EBUSY:			return "pth__mutex_lock: Mutex was already locked.";
	case EAGAIN:			return "pth__mutex_lock: Recursive lock limit exceeded.";
	case EDEADLK:			return "pth__mutex_lock: Deadlock, or mutex already owned by thread.";
	default:			return "pth__mutex_lock: Undocumented error return from pthread_mutex_lock()";
    }
}
//
char*  pth__mutex_trylock   (Mutex* mutex, Bool* result)   {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_lock.html
    // ==================
    //
    int err =  pthread_mutex_trylock( mutex );
    //
    switch (err) {
	//
	case 0: 	*result = FALSE;	return NULL;					// Successfully acquired lock.
	case EBUSY:	*result = TRUE;		return NULL;					// Lock was already taken.
	//
	default:				return "pth__mutex_trylock: Error while attempting to test lock.";
    }
}
//
char*  pth__mutex_unlock   (Mutex* mutex) {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_lock.html
    // =================
    //
    //
    int err =  pthread_mutex_unlock( mutex );
    //
    switch (err) {
	//
	case 0: 				return NULL;					// Successfully acquired lock.
	case EINVAL:				return "pth__mutex_unlock: Mutex has PTHREAD_PRIO_PROTECT set and calling thread's priority is higher than mutex's priority ceiling.";
	case EBUSY:				return "pth__mutex_unlock: The mutex already locked.";
	//
	default:				return "pth__mutex_unlock: Undocumented error returned by pthread_mutex_unlock().";
    }
}

//
char*    pth__condvar_init ( Condvar* condvar ) {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_init.html
    //   =================
    //
    if (pthread_cond_init( condvar, NULL ))	return "pth__condvar_init: Unable to initialize condition variable.";
    else					return NULL;
}
//
char*  pth__condvar_destroy   (Condvar* condvar) {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_init.html
    // ====================
    //
    if (pthread_cond_destroy( condvar ))	return "pth__condvar_destroy: Unable to destroy condition variable.";
    else					return NULL;
}
//
char*  pth__condvar_wait   (Condvar* condvar, Mutex* mutex) {			// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_wait.html
    // =================
    //
    if (pthread_cond_wait( condvar, mutex )) 	return "pth__condvar_wait: Unable to wait on condition variable.";
    else					return NULL;
}
//
char*  pth__condvar_signal   (Condvar* condvar) {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_signal.html
    // ===================
    //
    if (pthread_cond_signal( condvar ))		return "pth__condvar_signal: Unable to signal on condition variable.";
    else					return NULL;
}
//
char*  pth__condvar_broadcast   (Condvar* condvar) {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_signal.html
    // ======================
    //
    if (pthread_cond_broadcast( condvar ))	return "pth__condvar_broadcast: Unable to broadcast on condition variable.";
    else					return NULL;
}

//
char*  pth__barrier_init   (Barrier* barrier, int threads) {			// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_barrier_init.html
    // =================
    //
    if (pthread_barrier_init( barrier, NULL, (unsigned) threads))	return "pth__barrier_init: Unable to set barrier.";
    else								return NULL;
}
//
char*  pth__barrier_destroy   (Barrier* barrier) {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_barrier_init.html
    // ====================
    //
    if (pthread_barrier_destroy( barrier )) 	return "pth__barrier_init: Unable to clear barrier.";
    else					return NULL;
}

//
char*  pth__barrier_wait   (Barrier* barrier, Bool* result) {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_barrier_wait.html
    // =================
    //
    int err =  pthread_barrier_wait( barrier );
    //
    switch (err) {
	//
	case PTHREAD_BARRIER_SERIAL_THREAD:	*result = TRUE;		return NULL;								// Exactly one pthread gets this return value when released from barrier.
	case 0:					*result = FALSE;	return NULL;								// All other threads at barrier get this.
	default:							return "pth__barrier_wait: Fatal error while blocked at barrier.";
    }
}


//
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
    return  pthread_self();

    // Later: Looks like the official fn to use here is
    //
    //     pthread_t pthread_self(void); 		// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_self.html
    //
    // I don't know if getpid() is actually equivalent or not.   -- 2011-11-09 CrT
    // pthread_t appears to be "unsigned long int"
}
//
Pthread*  pth__get_pthread   ()   {
    //    ================
    //
    // Return Pthread* for currently running pthread -- this
    // is needed to find record for current pthread in contexts
    // like signal handlers where it is not (otherwise) available.
    //    
    //
#if NEED_PTHREAD_SUPPORT
    int pid =  pth__get_pthread_id ();							// Since this just calls pthread_self(), the result is available in all contexts.  (That we care about. :-)
    //
    for (int i = 0;  i < MAX_PTHREADS;  ++i) {
	//
	if (pthread_table__global[i]->pid == pid)   return pthread_table__global[ i ];	// pthread_table__global	def in   src/c/main/runtime-state.c
    }											// pthread_table__global exported via    src/c/h/runtime-pthread.h
    die ("pth__get_pthread:  pid %d not found in pthread_table__global?!", pid);
    return NULL;									// Cannot execute; only to quiet gcc.
#else
    //
    return pthread_table__global[ 0 ];
#endif
}

//
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
  
    pth__mutex_lock( &pthread_table_mutex__local );					// What could go wrong here if we didn't use a mutex...?
	//										// (Seems like reading a refcell is basically atomic anyhow.)
											// Late: Maybe if someone holds the lock, we want to wait
											// until they release it before reading the refcell.
        int active_pthread_count = TAGGED_INT_TO_C_INT( DEREF(ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL) );
	//
    pth__mutex_unlock ( &pthread_table_mutex__local );

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


// Resources:
//
// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_create.html
// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_exit.html

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

// pthread_barrier_init(&barr, NULL, THREADS)	// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_barrier_init.html
//     int rc = pthread_barrier_wait(&barr);    // http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_barrier_wait.html
// if(rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD)
//   {
//    printf("Could not wait on barrier\n");
//    exit(-1);
//   }
//

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


