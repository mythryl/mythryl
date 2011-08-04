// runtime-multicore.h


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

	typedef ulock_t	Lock;		// A lock.
	typedef barrier_t	Barrier;	// A barrier.
	typedef int 	Pid;		// A process id.

    #else
        #error MP not supported for this system
    #endif

    ////////////////////////////////////////////////////////////////////////////
    // Generic multicore interface.
    //
    // These are the system-dependent functions which we re-implement
    // for each supported system.

    extern int   mc_start_cleaning    (Task*);
    extern void  mc_finish_cleaning   (Task*, int);

    extern Val*  mc_extra_cleaner_roots_global [];

    extern void  mc_acquire_lock	(Lock lock);
    extern void  mc_release_lock	(Lock lock);
    extern Bool  mc_try_lock		(Lock lock);
    extern Lock  mc_make_lock		();
    extern void  mc_free_lock		(Lock lock);			// This call was probably only needed for SGI's daft hardware locks, and can be eliminated now. XXX BUGGO FIXME

    extern Barrier* mc_make_barrier 	();
    extern void     mc_free_barrier	(Barrier* barrierp);
    extern void     mc_barrier		(Barrier* barrierp, unsigned n);
    extern void     mc_reset_barrier	(Barrier* barrierp);

    extern Pid      mc_pthread_id		(void);
    extern int      mc_max_pthreads		();
    extern Val      mc_acquire_pthread		(Task* task,  Val arg);
    extern void     mc_release_pthread		(Task* task);
    extern int      mc_active_pthread_count	();
    extern void     mc_initialize		(void);
    extern void     mc_shut_down		(void);

    extern Lock	    mc_cleaner_lock_global;
    extern Lock	    mc_cleaner_gen_lock_global;
    extern Lock	    mc_timer_lock_global;
    extern Barrier* mc_cleaner_barrier_global;

    #define BEGIN_CRITICAL_SECTION( LOCK )	{ mc_acquire_lock(LOCK); {
    #define END_CRITICAL_SECTION( LOCK )	} mc_release_lock(LOCK); }
    #define ACQUIRE_LOCK(LOCK)		mc_acquire_lock(LOCK);
    #define RELEASE_LOCK(LOCK)		mc_release_lock(LOCK);

#endif // MULTICORE_SUPPORT

#endif // RUNTIME_MULTICORE_H



// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

