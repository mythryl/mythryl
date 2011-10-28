// runtime-pthread.h
//
// Support for multicore operation.
// This stuff is (in part) exported to
// the Mythrl world as
//
//     src/lib/std/src/pthread.api
//     src/lib/std/src/pthread.pkg
//
// via
//
//     src/c/lib/pthread/libmythryl-pthread.c
//
// Platform-specific implementations of
// this functionality are:
//
//     src/c/pthread/sgi-multicore.c
//     src/c/pthread/pthread-on-solaris.c

#ifndef RUNTIME_MULTICORE_H
#define RUNTIME_MULTICORE_H

#include "../config.h"

// Status of a Pthread:
//
typedef enum {
    //
    KERNEL_THREAD_IS_RUNNING,
    KERNEL_THREAD_IS_SUSPENDED,
    NO_KERNEL_THREAD_ALLOCATED
    //
} Pthread_Status;

#ifndef MULTICORE_SUPPORT

    #define BEGIN_CRITICAL_SECTION( LOCK )	{
    #define END_CRITICAL_SECTION( LOCK )	}
    #define ACQUIRE_LOCK(LOCK)		// no-op
    #define RELEASE_LOCK(LOCK)		// no-op

#else // MULTICORE_SUPPORT

    #if !defined( SOFTWARE_GENERATED_PERIODIC_EVENTS ) \
     || !defined( MULTICORE_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS )
	#error Multicore runtime currently requires polling support.
    #endif

    ////////////////////////////////////////////////////////////////////////////
    // OS dependent stuff:
    //

    #if defined(OPSYS_IRIX5)

	#if HAVE_SYS_TYPES_H
	    #include <sys/types.h>
	#endif

	#include <sys/prctl.h>

	#if HAVE_UNISTD_H
	    #include <unistd.h>
	#endif

	#include <ulocks.h>

	typedef ulock_t	Lock;			// A lock.
	typedef barrier_t	Barrier;	// A barrier.
	typedef int 	Pid;			// A process id.

    #else
        #error MP not supported for this system
    #endif



    ////////////////////////////////////////////////////////////////////////////
    // PACKAGE STARTUP AND SHUTDOWN
    //
    extern void     mc_initialize		(void);					// Called once near the top of main() to initialize the package.  Allocates our static locks, may also mmap() memory for arena or whatever.
    extern void     mc_shut_down		(void);					// Called once just before calling exit(), to release any OS resources.



    ////////////////////////////////////////////////////////////////////////////
    // PTHREAD START/STOP/ETC SUPPORT
    //
    extern Val      mc_acquire_pthread		(Task* task,  Val arg);			// Called with (thread, closure) and if a pthread is available starts arg running on a new pthread and returns TRUE.
    //											// Returns FALSE if we're already maxed out on allowed number of pthreads.
    //											// This gets exported to the Mythryl level as "pthread"::"acquire_pthread"  via   src/c/lib/pthread/cfun-list.h
    //											// There is apparently currently no .pkg file referencing this value.
    //
    extern void     mc_release_pthread		(Task* task);				// Reverse of above, more or less.
    //											// On Solaris this appears to actually stop and kill the thread.
    //											// On SGI this appears to just suspend the thread pending another request to run something on it.
    //											// Presumably the difference is that thread de/allocation is cheaper on Solaris than on SGI...?
    // 
    extern Pid      mc_pthread_id		(void);					// Supplies value for pthread_table_global[0]->pid in   src/c/main/runtime-state.c
    //											// This just calls getpid()  in                         src/c/pthread/sgi-multicore.c
    //											// This returns thr_self() (I don't wanna know) in      src/c/pthread/pthread-on-solaris.c
    //
    extern int      mc_max_pthreads		();					// Just exports to the Mythryl level the MAX_PTHREADS value from   src/c/h/runtime-configuration.h
    //
    extern int      mc_active_pthread_count	();					// Just returns (as a C int) the value of   ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL, which is defined in   src/c/h/runtime-globals.h
											// Used only to set barrier for right number of pthreads in   src/c/heapcleaner/pthread-cleaning-stuff.c


    ////////////////////////////////////////////////////////////////////////////
    // MULTICORE GARBAGE COLLECTION SUPPORT
    //
    extern int   mc_start_cleaning    (Task*);
    extern void  mc_finish_cleaning   (Task*, int);
    //
    extern Val*  mc_extra_cleaner_roots_global [];



    ////////////////////////////////////////////////////////////////////////////
    //                   LOCKS
    //
    // We use our "locks" to perform mutual exclusion,
    // ensuring consistency of shared mutable datastructures
    // by ensuring that at most one pthread at a time is
    // updating that datastructure.  Typically we allocate
    // one such lock for each major shared mutable datastructure,
    // which persists for as long as that datastructure.
    //
    extern Lock  mc_make_lock		();					// Just what you think.
    extern void  mc_free_lock		(Lock lock);				// This call was probably only needed for SGI's daft hardware locks, and can be eliminated now. XXX BUGGO FIXME
    //
    extern void  mc_acquire_lock	(Lock lock);				// Used to enter a critical section, preventing any other pthread from proceeding past mc_acquire_lock() for this lock until we release.
    extern void  mc_release_lock	(Lock lock);				// Reverse of preceding operation; exits critical section and allows (one) other pthread to proceed past mc_acquire_lock() on this lock.
    //
    extern Bool  mc_try_lock		(Lock lock);				// This appears to be a non-blocking variant of mc_acquire_lock, which always returns immediately with either TRUE (lock acquired) or FALSE.
    //
    // Some statically pre-allocated locks:
    //
    extern Lock	    mc_cleaner_lock_global;
    extern Lock	    mc_cleaner_gen_lock_global;
    extern Lock	    mc_timer_lock_global;
    extern Barrier* mc_cleaner_barrier_global;
    //
    // Some readability tweaks:
    //
    #define BEGIN_CRITICAL_SECTION( LOCK )	{ mc_acquire_lock(LOCK); {
    #define END_CRITICAL_SECTION( LOCK )	} mc_release_lock(LOCK); }
    #define ACQUIRE_LOCK(LOCK)		mc_acquire_lock(LOCK);
    #define RELEASE_LOCK(LOCK)		mc_release_lock(LOCK);


    ////////////////////////////////////////////////////////////////////////////
    //                   BARRIERS
    //
    // We use our "barriers" to perform essentially the
    // opposite of mutual exclusion, ensuring that all
    // pthreads in a set have completed their part of
    // a shared task before any of them are allowed to
    // proceed past the "barrier".
    //
    // Our only current use of this facility is in
    //
    //     src/c/heapcleaner/pthread-cleaning-stuff.c
    //
    // where it serves to ensure that garbage collection
    // does not start until all pthreads have ceased normal
    // processing, and that no pthread resumes normal processing
    // until the garbage collection is complete.
    //
    // The literature distinguishes barriers where waiting is
    // done by blocking from those where waiting is done by spinning;
    // it isn't clear which was intended by the original authors.
    //
    // NB: This facility seems to be implemented directly in hardware in    src/c/pthread/sgi-multicore.c
    // but implemented on top of locks in                                   src/c/pthread/pthread-on-solaris.c
    //
    extern Barrier* mc_make_barrier 	();					// Allocate a barrier.
    extern void     mc_free_barrier	(Barrier* barrierp);			// Free a barrier.
    //
    extern void     mc_barrier		(Barrier* barrierp, unsigned n);	// Should be called 'barrier_wait' or such.  Block pthread until 'n' pthreads are waiting at the barrier, then release them all.
    //										// It is presumed that all threads waiting on a barrier use the same value of 'n'; otherwise behavior is probably undefined. (Poor design IMHO.)
    //
    extern void     mc_reset_barrier	(Barrier* barrierp);			// (Never used.)  Reset barrier to initial state. Presumably any waiting pthreads are released to proceed.





#endif // MULTICORE_SUPPORT

#endif // RUNTIME_MULTICORE_H



// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

