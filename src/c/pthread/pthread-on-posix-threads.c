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



/*
###                "Light is the task when many share the toil."
###
###                                 -- Homer, circa 750BC
*/



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
#include "runtime-globals.h"
#include "heapcleaner.h"


#define INCREASE_BY(n,i)  ((Val)TAGGED_INT_FROM_C_INT(TAGGED_INT_TO_C_INT(n) + (i)))
#define DECREASE_BY(n,i)  (INCREASE_BY(n,(-i)))


//
int   pth__done_pthread_create  =  TRUE;		// This is currently always TRUE -- see Note[1] at bottom of file.
    //========================


// Some statically allocated locks.
//
// We try to put each mutex in its own cache line
// to prevent cores thrashing against each other
// trying to get control of logically unrelated mutexs:
//
// It would presumably be good to force cache-line-size
// alignment here, but I don't know how, short of	// LATER: But see synopsis of posix_memalign in src/c/lib/pthread/libmythryl-pthread.c
// malloc'ing and checking alignment at runtime:
/**/													char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];

        // We place these two here in the hope
        // that they might wind up in the same
	// cache line as the mutex governing them
	// -- that should buy us a little efficiency:
	//
        int  pth__it_is_heapcleaning_time =  FALSE;							// Do NOT read or write this unless holding   pth__pthread_mode_mutex.
        int  pth__running_pthreads_count  =  1;								// Do NOT read or write this unless holding   pth__pthread_mode_mutex.
		//
		// Should always equal the number of pthreads
		// with pthread->mode == IS_RUNNING.
        Mutex	 pth__pthread_mode_mutex		= PTHREAD_MUTEX_INITIALIZER;			// No padding here because it might as well share a cache line with next.
                    //
		    // Must hold this mutex in order to read or write
		    //     pthread->mode fields or global flag
		    //     pth__it_is_heapcleaning_time
		    //     pth__running_pthreads_count
		    // When setting pth__running_pthreads_count to zero,
		    // must signal pth__no_running_pthreads_condvar
		    // if pth__it_is_heapcleaning_time
		    // is TRUE -- this lets the primary heapcleaner pthread
		    // start heapcleaning.

        Mutex	 pth__blocked_to_running_mutex		= PTHREAD_MUTEX_INITIALIZER;			// No padding here because it might as well share a cache line with next.
		    //
		    // A pthread must hold this while switching from
		    // pthread->mode == IS_BLOCKED to
		    // pthread->mode == IS_RUNNING mode.
		    // The primary heapcleaning pthread holds this during
 		    // garbage collection to prevent blocked threads from
		    // waking up and attempting to use the Mythryl heap.

        Condvar	 pth__no_running_pthreads_condvar	= PTHREAD_COND_INITIALIZER;		
		    //
		    // The primary heapcleaner pthread (i.e., the pthread
		    // that decided to initiate heapcleaning) waits on
		    // this before starting heapcleaning, to allow all
		    // pthread->mode == IS_RUNNING pthreads to exit
		    // running mode.	

        Mutex	 pth__heapcleaner_mutex			= PTHREAD_MUTEX_INITIALIZER;		char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];	// Used only in   src/c/heapcleaner/pthread-heapcleaner-stuff.c
        Mutex	 pth__heapcleaner_gen_mutex		= PTHREAD_MUTEX_INITIALIZER;		char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];	// Used only in   src/c/heapcleaner/make-strings-and-vectors-etc.c
        Mutex	 pth__timer_mutex			= PTHREAD_MUTEX_INITIALIZER;		char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];	// Apparently never used.
static  Mutex	      pthread_table_mutex__local	= PTHREAD_MUTEX_INITIALIZER;		char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];	// Used in this file to serialize access to pthread_table__global[].



//
Barrier  pth__heapcleaner_barrier;					// Used only with pth__wait_at_barrier prim, in   src/c/heapcleaner/pthread-heapcleaner-stuff.c





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
										// struct task				def in   src/c/h/runtime-base.h

char* pth__pthread_create   (int* pthread_table_slot, Val current_thread, Val closure_arg)   {
    //===================
    //
    // Run 'closure_arg' in its own kernel thread.
    //
    // This fn is called (only) by   spawn_pthread ()   in   src/c/lib/pthread/libmythryl-pthread.c
    //
    pth__done_pthread_create = TRUE;					// Once set TRUE, this is never set back to FALSE. (So locking is not an issue.)

    Task*    task;
    Pthread* pthread;
    int      i;

    PTHREAD_LOG_IF ("[Searching for free pthread]\n");

    pth__mutex_lock( &pthread_table_mutex__local );				// Always first step before reading/writing pthread_table__global.
										// We don't use the PTH__MUTEX_LOCK macro because at this point
										// we know pth__done_pthread_create == TRUE.
    //
    if (DEREF( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL ) == TAGGED_INT_FROM_C_INT( MAX_PTHREADS )) {
	//
	pth__mutex_unlock( &pthread_table_mutex__local );
	return "pthread_table__global full -- increase MAX_PTHREADS";
    }

    // Search for a slot in which to put a new pthread
    //
    for (i = 0;
	(i < MAX_PTHREADS)  &&  (pthread_table__global[i]->mode != IS_VOID);
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

    pthread->mode = IS_RUNNING;							// Moved this above pthread_create() because that seems safer,
										// otherwise child might run arbitrarily long without this being set. -- 2011-11-10 CrT
    int err =   pthread_create(
		    //
		    &task->pthread->tid,					// RESULT. NB: Passing a pointer directly to task->pthread->tid ensures that field is always
										//         valid as seen by both parent and child threads, without using spinlocks or such.
										//	   Passing the pointer is safe (only) because 'tid' is of type pthread_t from <pthread.h>
										//	   -- we define field 'tid' as 'Tid' in src/c/h/pthread.h
										//	   and   typedef pthread_t Tid;   in   src/c/h/runtime-base.h

		    NULL,							// Provision for attributes -- API futureproofing.

		    pthread_main,  (void*) task					// Function + argument to run in new kernel thread.
		);

    if (!err) {									// Successfully spawned new kernel thread.
	//
	return NULL;								// Report success. NB: Child thread (i.e., pthread_main() above)  will unlock  pthread_table_mutex__local  for us.

    } else {									// Failed to spawn new kernel thread.

	pthread->mode = IS_VOID;						// Note pthread record (still) has no associated kernel thread.

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
	//													// we know pth__done_pthread_create == TRUE.
	task->pthread->mode = IS_VOID;
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

    if (pthread->mode == IS_VOID)  {
	//
	return "pth__pthread_join: Bogus value for pthread-to-join (already-dead thread?)";
    }

    int     err =  pthread_join( pthread->tid, NULL );				// NULL is a void** arg that can return result of joined thread. We ignore it
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
    int err =  pthread_mutex_lock( mutex );
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

// Here's a little tutorial on posix condition variables:
//
//     http://www.gentoo.org/doc/en/articles/l-posix3.xml
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
Tid   pth__get_pthread_id   ()   {
    //===================
    //
    // Return a unique small-int id distinguishing
    // the currently running pthread from all other
    // pthreads.
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
    int tid =  pth__get_pthread_id ();							// Since this just calls pthread_self(), the result is available in all contexts.  (That we care about. :-)
    //
    for (int i = 0;  i < MAX_PTHREADS;  ++i) {
	//
	if (pthread_table__global[i]->tid == tid)   return pthread_table__global[ i ];	// pthread_table__global	def in   src/c/main/runtime-state.c
    }											// pthread_table__global exported via    src/c/h/runtime-base.h
    die ("pth__get_pthread:  tid %d not found in pthread_table__global?!", tid);
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



///////////////////////////////////////////////////////////////////////////////////////////
//
void release_mythryl_heap(  Pthread* pthread,  const char* fn_name,  Val* arg  ) {
    //
    pthread_mutex_lock(  &pth__pthread_mode_mutex  );

	// Remove us from the set of RUNNING pthreads:
	//
	pthread->mode = IS_BLOCKED;
	--pth__running_pthreads_count;

	// Protect 'arg' from the heapcleaner by making it a heapcleaner root:
	//
	pthread->task->protected_c_arg = arg;

	// If primary heapcleaning pthread can run now, tell it so:
	//
	if (pth__it_is_heapcleaning_time
	&&  pth__running_pthreads_count == 0
	){
	    pthread_cond_signal( &pth__no_running_pthreads_condvar );
	}
    pthread_mutex_unlock(  &pth__pthread_mode_mutex  );
}

void recover_mythryl_heap(  Pthread* pthread,  const char* fn_name  ) {
    //
    // First mutex is to prevent a BLOCKED pthread from
    // resuming execution during a heapcleaning.

    // NB: Must always acquire these two mutexes in the
    // same order, to prevent deadlock!

    pthread_mutex_lock(   &pth__blocked_to_running_mutex  );
	pthread_mutex_lock(   &pth__pthread_mode_mutex  );

	    // Return us to the set of RUNNING pthreads:
	    //
	    pthread->mode = IS_RUNNING;
	    ++pth__running_pthreads_count;

	    // Make 'arg' no longer be a heapcleaner root:
	    //
	    pthread->task->protected_c_arg = &pthread->task->heapvoid;

	pthread_mutex_unlock(   &pth__pthread_mode_mutex   );
    pthread_mutex_unlock(   &pth__blocked_to_running_mutex  );
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

// Overview
// ========
//
// Posix-thread support for Mythryl.  The original work this
// is based on was
//
//      A Portable Multiprocessor Interface for Standard ML of New Jersey 
//      Morrisett + Tolmach 1992 31p 
//      http://handle.dtic.mil/100.2/ADA255639
//      http://mythryl.org/pub/pml/a-portable-multiprocessor-interface-for-smlnj-morrisett-tolmach-1992.ps 
//
// The ultimate design goal is to support multiple cores
// executing Mythryl code in parallel on a shared Mythryl
// heap with speedup linear in the number of cores.
//
// In practice, initially, my focal goal is more modest:			// "me" being Cynbe 2011-11-24
// to support one pthread executing threadkit ("Concurrent ML")
// code at full speed while other pthreads offload I/O-bound
// and CPU-bound processing.
//
// The central problems are
//
//   1) How to maintain Mythryl heap coherency in the
//      face of multiple kernel threads executing Mythryl
//      code in parallel while retaining good performance.
//
//   2) How to maintain Mythryl heap coherency during
//      heapcleaning ("garbage collection").
//
// The matching solutions they adopted are:
//
//   1) Most heap allocation is done in generation zero;
//      by giving each kernel thread its own independent
//      generation zero, each can allocate at full speed
//      without locking overhead.
//
//      Allocation is also done directly into later heap
//      generations, but this happens too seldom to be
//      performance critical, so conventional mutex locking
//      can be used without problem.
//      
//   2) Parallel garbage collection is nontrivial;  for ease
//      of implementation (and debugging!) the existing
//      garbage collector is used running on a single kernel
//      thread.
//
//
// Point (2) requires further analysis and design effort.
//
// The central problem is that (with the current algorithm)
// heapcleaning cannot be done while Mythryl pthreads are
// reading or writing the Mythryl heap.  Consequently before
// heapcleaning can begin, all pthreads must be signalled to
// suspend use of the Mythryl heap and must be confirmed to
// have done so.
//
// Furthermore, each pthread must be brought to a halt with
// its private generation-zero buffer in a consistent state
// intelligible to the heapcleaner; in particular there can
// be no allocated but uninitialized pointers in the heap
// containing nonsense values which might make the heapcleaner
// segfault.
//
// The Mythryl compiler guarantees that all Mythryl code
// frequently runs the out-of-heap-space probe logic -- in
// particular that every closed loop through the code contains
// at least one such probe call.  It also guarantees that such
// probes calles are done only at the start of execution of
// a function, when the heap is in a self-consistent state.
// It is therefor simple and natural to take advantage of this
// mechanism by having these heap-limit probe calls check a
// global 'pth__it_is_heapcleaning_time' flag and (if it is
// TRUE) suspend execution of Mythryl code and enter a special
// "heapcleaning mode".
// 
// This solves half our problem:  When it is time to do a
// heapcleaning, we set
//     pth__it_is_heapcleaning_time = TRUE
// and wait for all running Mythryl pthreads to stop running
// and enter heapcleaning mode.
//
// The remaining half of the problem is that some pthreads
// may be blocked on a system call like sleep() or select()
// and might not notice that
//     pth__it_is_heapcleaning_time
// is TRUE for milliseconds -- or even seconds, minutes or hours.
// We run the heapcleaner about 200 times per second, and
// it cannot start until all pthreads are known not to be
// reading or writing the Mythryl heap, so waiting for all
// system calls to return is out of the question.
//
// Our solution is to distinquish three different pthread 'modes':
//
//    IS_RUNNING:    Pthread is actively running Mythryl code
//                   and reading and writing the Mythryl heap;
//                   it can be counted upon to respond quickly if
//                   pth__it_is_heapcleaning_time is set TRUE
//                   (where "quickly" means microseconds not
//                   milliseconds).
//
//    IS_BLOCKED:    Pthread may be blocked indefinitely in
//                   a sleep() or select() or such and thus
//                   cannot be counted upon to respond quickly if
//                   pth__it_is_heapcleaning_time is set TRUE,
//                   but it is guaranteed not to be reading
//                   or writing the Mythryl heap, or to
//                   contain any hidden pointers into the
//                   Mythryl heap, so heapcleaning can proceed
//                   safely.
//
//    IS_HEAPCLEANING:
//                   Pthread has detected that the global
//                   pth__it_is_heapcleaning_time flag is TRUE
//                   and has responded by ceasing execution of
//                   user Mythryl code and entered into a quiescent
//                   state allowing heapcleaning to proceed.
//                   
// 
// The idea is then that when heapcleaning is necessary we
// 
//   1) Set pth__it_is_heapcleaning_time to TRUE.
//      and prevent BLOCKED pthreads from entering RUNNING mode.
// 
//   2) Wait until the number of pthreads with
//      pthread->mode ==  IS_RUNNING drops to zero.
// 
//   3) Set pth__it_is_heapcleaning_time to FALSE.
//
//   4) Clean the heap. (This may result in some or all
//      records on the heap moving to new addresses.)
// 
//   5) Allow all IS_HEAPCLEANING pthreads to return to
//      IS_RUNNING mode and IS_BLOCKED pthreads to re-enter
//      IS_RUNNING if they wish.
//
//
//
// To implement this high-level plan we introduce the following
// detailed mechanisms and policies:
//
//
//   X  To distinguish pthread modes we introduce a type
//
//         Pthread_Mode = IS_RUNNING		// Pthread is running Mythryl code -- will respond quickly to 
//                      | IS_BLOCKED
//                      | IS_HEAPCLEANING
//
//      in   src/c/h/runtime-base.h
//
//
//   X  To record our per-thread state we introduce a field
//
//	    pthread->mode
//
//      of type Pthread_Mode in the
//      pthread_state_struct def in   src/c/h/runtime-base.h
//      
//
//   X  To signal RUNNING pthreads to enter HEAPCLEANING
//      mode we introduce a boolean
//
//          pth__it_is_heapcleaning_time;
//
//      in   src/c/pthread/pthread-on-posix-threads.c
//
// ==>   We set this flag TRUE in   src/c/heapcleaner/pthread-heapcleaner-stuff.c
//      when we want all IS_RUNNING pthreads to switch to IS_HEAPCLEANING mode;
//
//      Function   need_to_call_heapcleaner() in   src/c/heapcleaner/call-heapcleaner.c
//      checks this and returns TRUE immediately if it is set.
//
//
//   o  We introduce an int
//
//          pth__running_pthreads_count
//
//      in   src/c/pthread/pthread-on-posix-threads.c
//      which is always equal to the number of pthreads with
//      pthread->mode == IS_RUNNING.
//      Heapcleaning cannot begin until this count reaches zero, 
//
//
//   X  We introduce a Mutex
//
//          pth__pthread_mode_mutex
//
//      in   src/c/pthread/pthread-on-posix-threads.c
//      to govern
//
//           pthread->mode
//           pth__it_is_heapcleaning_time
//           pth__running_pthreads_count
//
//      This mutex must be held when making any change to
//      these state variables --  this includes in particular
//      creating or destroying pthreads.  (It is usually also
//      safest hold this mutex when reading those variables.)
//
//
//   o  We introduce a Condvar
//
//          pth__no_running_pthreads_condvar
//
//      in   src/c/pthread/pthread-on-posix-threads.c
//      which the primary heapcleaning pthread waits
//      on after setting pth__it_is_heapcleaning_time
//      to TRUE and before starting heapcleaning;
//      the last  running pthread signals this condvar
//      as it exits pthread->mode == IS_RUNNING mode.
//
//
//   o  We introduce a Mutex
//
//          pth__blocked_to_running_mutex
//
//      in   src/c/pthread/pthread-on-posix-threads.c
//      which a pthread must hold to change pthread->mode
//      from IS_BLOCKED to IS_RUNNING.  The point of this
//      is that the primary heapcleaner process can
//      hold it while heapcleaning is in progress to
//      prevent blocked pthreads from waking up and
//      attempting to read/write the half-cleaned heap.
//
//
//   o  We introduce a pair of macros
//
//          RELEASE_MYTHRYL_HEAP( pthread, "fn_name", arg );
//              //
//              syscall();
//              //
//          RECOVER_MYTHRYL_HEAP( pthread, "fn_name"      );
//
//      in   src/c/h/runtime-base.h
//      to serve as brackets around every system call.
//      
//      (Here 'arg' is the Val argument passed from Mythryl
//      to the C function making the system call -- 'arg'
//      generally needs to be protected against garbage collection.)
//      
//      The basic idea is for these macros to respectively do:
//      
//          RELEASE_MYTHRYL_HEAP:
//             pthread_mutex_lock(  &pth__pthread_mode_mutex  );
//                 pthread->mode = IS_BLOCKED;							// Remove us from the set of RUNNING pthreads.
//                 --pth__running_pthreads_count;
//                 pthread->task->protected_c_arg = &(arg);					// Protect 'arg' from the heapcleaner by making it a heapcleaner root.
//                 if (pth__it_is_heapcleaning_time
//                 &&  pth__running_pthreads_count == 0
//                 ){
//                     pthread_cond_signal( &pth__no_running_pthreads_condvar );		// This lets the primary heapcleaning pthread start heapcleaning.	
//                 }
//             pthread_mutex_unlock(  &pth__pthread_mode_mutex  );
//      
//          RECOVER_MYTHRYL_HEAP:
//             pthread_mutex_lock(   &pth__blocked_to_running_mutex  );				// This prevents a blocked pthread from resuming execution during a heapcleaning.
//                 pthread_mutex_lock(   &pth__pthread_mode_mutex  );				// Must always acquire these two locks in the same order to prevent deadlock!
//                     pthread->mode = IS_RUNNING;						// Return us to the set of RUNNING pthreads.
//                     ++pth__running_pthreads_count;
//                     pthread->task->protected_c_arg = &pthread->task->heapvoid;		// Make 'arg' no longer a heapcleaner root.
//                 pthread_mutex_unlock(   &pth__pthread_mode_mutex   );
//             pthread_mutex_unlock(   &pth__blocked_to_running_mutex  );
//
//
//   o  The logic for initiating a heapcleaning is then:
//
//         pthread_mutex_lock(   &pth__blocked_to_running_mutex  );				// Stop IS_BLOCKED pthreads from exiting blocked mode for duration of garbage collection.
//             pthread_mutex_lock(   &pth__pthread_mode_mutex  );				// Must always acquire these two locks in the same order to prevent deadlock!
//
//                 if (pth__running_pthreads_count == 1) {
//                     do heapcleaning								// With no other running threads we can just do it.
//                     pthread_mutex_unlock(   &pth__pthread_mode_mutex  );
//                     pthread_mutex_unlock(   &pth__blocked_to_running_mutex  );		// Allow IS_BLOCKED pthreads to exit blocked mode at will.
//                     return;
//                 }
//
//                 pthread_barrier_init(							// Set up barrier for other IS_RUNNING pthreads to wait at while heapcleaning is in progress. 
//                     &pth__heapcleaner_barrier,
//		       &pth__running_pthreads_count
//                 );
//
//                 pthread->mode = IS_HEAPCLEANING;						// Remove ourself from the set of IS_RUNNING pthreads.
//                 --pth__running_pthreads_count;
//
//                 pth__it_is_heapcleaning_time = TRUE;						// Signal other running threads to enter heapcleaning mode.
//
//                 pthread_cond_wait(								// Wait until no running pthreads are left.
//                    &pth__no_running_pthreads_condvar,
//                    &pth__pthread_mode_mutex							// Release this while waiting, so IS_RUNNING pthreads can enter IS_HEAPCLEANING mode.
//                 );
//
//                 pth__it_is_heapcleaning_time = FALSE;					// Clear the enter-heapcleaning-mode signal.
//
//                 // NB: At this point we again hold pth__pthread_mode_mutex.
//                 do heapcleaning
//
//                 pthread_barrier_wait(  &pth__pthread_mode_mutex  );				// Release other IS_HEAPCLEANING pthreads to resume IS_RUNNING mode.
//                 pthread_barrier_destroy( &pth__pthread_mode_mutex  );			// Return barrier to original unconfigured mode.

//                 pthread->mode = IS_RUNNING;
//                 ++pth__running_pthreads_count;
//
//             pthread_mutex_unlock(  &pth__pthread_mode_mutex  );
//         pthread_mutex_unlock*   &pth__blocked_to_running_mutex  );				// Allow IS_BLOCKED pthreads to exit blocked mode at will.
//
//
//   o  The logic for a secondary heapcleaning process is then
//
//      if (pth__it_is_heapcleaning_time) {
//          pthread_mutex_lock(  &pth__pthread_mode_mutex  );
//          pthread->mode = IS_HEAPCLEANING;
//          --pth__running_pthreads_count;
//          if (pth__running_pthreads_count == 0) {
//              pthread_mutex_unlock(  &pth__pthread_mode_mutex  );
//          } else {
//             pthread_mutex_unlock(  &pth__pthread_mode_mutex  );
//             pthread_cond_broadcast(  &pth__no_running_pthreads_condvar  );			// Signal primary heapcleaner pthread to start heapcleaning.
//          }
//          pthread_barrier_wait(  &pth__pthread_mode_mutex  );					// Wait until heapcleaning is complete
//          pthread_mutex_lock(  &pth__pthread_mode_mutex  );
//              thread->mode = IS_RUNNING;
//              ++pth__running_pthreads_count;
//          pthread_mutex_unlock(  &pth__pthread_mode_mutex  );
//      }
//
// NB: We never actually check the pthread->mode values;
// they could actually be dispensed with.  But it seems a
// good idea to maintain them anyhow for for debugging and
// documentation purposes.
// 
//
//
// This is strictly a toe-in-the-water minimal implementation
// of Mythryl multiprocessing.  Cheng's 2001 thesis CU shows
// how to do an all-singing all-dancing all-scalable all-parallel
// implementation:  See  http://mythryl.org/pub/pml/
//
// The critical files for this facility are:
//
//     src/c/lib/pthread/libmythryl-pthread.c		Our Mythryl<->C world interface logic.
//
//     src/c/pthread/pthread-on-posix-threads.c          Our interface to the <ptheads.h> library proper.
//
//     src/c/h/runtime-base.h                            Contains our API for the previous file.
//
//     src/c/heapcleaner/pthread-heapcleaner-stuff.c	Added logic to stop all posix threads before starting
//							garbage collection and restart them after it is complete.
//
//     src/c/heapcleaner/call-heapcleaner.c		(Pre-existing file): Tweaks to invoke previous file and
//							to cope with having garbage collector roots in multiple
//							posix threads instead of just one.
//
//     src/c/mythryl-config.h				Critical configuration stuff, in particular
//							 NEED_PTHREAD_SUPPORT and MAX_PTHREADS.
//
//     src/lib/std/src/pthread.api			Mythryl-programmer interface to posix-threads functionality.
//     src/lib/std/src/pthread.pkg			Implementation of previous; this is just wrappers for the calls
//							exported by src/c/lib/pthread/libmythryl-pthread.c

///////////////////////////////////////////////////////////////////////
// Note[1]
//
// int   pth__done_pthread_create  =  TRUE;
//
// The original idea was that this flag would start out FALSE
// and be set TRUE the first time   pth__pthread_create   is called,
// with the idea of testing it and not slowing down execution with
// mutex ops when only one posix thread was present.
//
// However, running
//
//     make rest ; sudo make install ; make cheg ; make tart ; time make compiler
//
// repeatedly as a test with the variable either TRUE or
// FALSE shows the compiler runs faster with it TRUE:
//      189.005 vs 202.191 usermode CPU seconds
//      (see raw times below).
// ??? !
//
// This makes no sense to me, but for the moment I'm
// leaving the flag set TRUE.           -- 2011-11-16 CrT
//
//
// With   pth__done_pthread_create  =  TRUE
// 
//     172.054u 25.889s 1:22.98 238.5%	0+0k 0+198424io 0pf+0w
//     181.971u 28.589s 1:33.54 225.0%	0+0k 0+198432io 0pf+0w
//     181.843u 30.141s 1:34.05 225.3%	0+0k 13568+198440io 49pf+0w
//     182.927u 29.921s 1:34.72 224.7%	0+0k 0+198432io 0pf+0w
//     183.839u 30.465s 1:35.30 224.8%	0+0k 32+198424io 1pf+0w
//     170.446u 26.333s 1:25.97 228.8%	0+0k 0+198424io 0pf+0w
//     188.907u 29.981s 1:37.95 223.4%	0+0k 0+198432io 0pf+0w
//     193.288u 32.094s 1:36.00 234.7%	0+0k 0+198448io 0pf+0w
//     182.943u 31.189s 1:32.16 232.3%	0+0k 13272+198448io 49pf+0w
//     173.330u 25.933s 1:30.31 220.6%	0+0k 2512+198440io 19pf+0w
//     180.719u 30.017s 1:32.09 228.8%	0+0k 0+198424io 0pf+0w
// 
// With   pth__done_pthread_create  =  FALSE
// 
//     217.049u 40.618s 1:49.69 234.8%	0+0k 3400+206800io 20pf+0w
//     200.368u 34.722s 1:32.49 254.1%	0+0k 16+198432io 0pf+0w
//     201.724u 35.274s 1:32.30 256.7%	0+0k 0+198448io 0pf+0w
//     198.660u 33.998s 1:32.96 250.2%	0+0k 0+198440io 0pf+0w
//     200.932u 33.886s 1:35.07 246.9%	0+0k 0+198432io 0pf+0w
//     201.112u 35.210s 1:34.14 251.0%	0+0k 0+198424io 0pf+0w
//     201.080u 34.926s 1:31.85 256.9%	0+0k 0+198440io 0pf+0w
//     199.300u 34.030s 1:31.47 255.0%	0+0k 0+198496io 0pf+0w
//     200.156u 34.022s 1:34.35 248.1%	0+0k 0+198440io 0pf+0w
//     201.528u 35.270s 1:34.38 250.8%	0+0k 0+198440io 0pf+0w
// 
// With   pth__done_pthread_create  =  TRUE (again)
// 
//     183.115u 28.801s 1:36.66 219.2%	0+0k 0+198424io 0pf+0w
//     195.156u 31.957s 1:37.67 232.5%	0+0k 0+198456io 0pf+0w
//     173.314u 26.349s 1:29.63 222.7%	0+0k 0+198440io 0pf+0w
//     181.591u 29.853s 1:29.85 235.3%	0+0k 0+198456io 0pf+0w
//     174.538u 26.825s 1:27.42 230.3%	0+0k 0+198480io 0pf+0w
//     169.238u 25.433s 1:27.40 222.7%	0+0k 0+198448io 0pf+0w
//     169.530u 25.473s 1:25.95 226.8%	0+0k 0+198456io 0pf+0w
//     179.911u 28.021s 1:31.67 226.8%	0+0k 0+198464io 0pf+0w
//     180.975u 28.893s 1:33.50 224.4%	0+0k 0+198456io 0pf+0w
//     180.471u 29.141s 1:28.72 236.2%	0+0k 0+198448io 0pf+0w


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


