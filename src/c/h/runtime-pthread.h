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
//     src/c/pthread/pthread-on-posix-threads.c
//     src/c/pthread/pthread-on-sgi.c
//     src/c/pthread/pthread-on-solaris.c

#ifndef RUNTIME_MULTICORE_H
#define RUNTIME_MULTICORE_H

#include "../mythryl-config.h"

#include "runtime-base.h"

// Status of a Pthread:
//
typedef enum {
    //
    PTHREAD_IS_RUNNING,
    PTHREAD_IS_SUSPENDED,
    NO_PTHREAD_ALLOCATED
    //
} Pthread_Status;



#ifndef NO_PTHREAD_SUPPORT	// Temporary hack -- should be NEED_PTHREAD_SUPPORT XXX BUGGO FIXME

    #if !NEED_SOFTWARE_GENERATED_PERIODIC_EVENTS \
     || !NEED_PTHREAD_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	//
	#error Multicore runtime currently requires polling support.
    #endif

    #if HAVE_SYS_TYPES_H
	#include <sys/types.h>
    #endif

    #include <sys/prctl.h>

    #if HAVE_UNISTD_H
	#include <unistd.h>
    #endif







    ////////////////////////////////////////////////////////////////////////////
    // PACKAGE STARTUP AND SHUTDOWN
    //
    extern void     pth__start_up		(void);					// Called once near the top of main() to initialize the package.  Allocates our static locks, may also mmap() memory for arena or whatever.
    extern void     pth__shut_down		(void);					// Called once just before calling exit(), to release any OS resources.



    ////////////////////////////////////////////////////////////////////////////
    // PTHREAD START/STOP/ETC SUPPORT
    //
    extern Val      pth__acquire_pthread		(Task* task,  Val arg);		// Called with (thread, closure) and if a pthread is available starts arg running on a new pthread and returns TRUE.
    //											// Returns FALSE if we're already maxed out on allowed number of pthreads.
    //											// This gets exported to the Mythryl level as "pthread"::"acquire_pthread"  via   src/c/lib/pthread/cfun-list.h
    //											// There is apparently currently no .pkg file referencing this value.
    //
    extern void     pth__release_pthread		(Task* task);				// Reverse of above, more or less.
    //											// On Solaris this appears to actually stop and kill the thread.
    //											// On SGI this appears to just suspend the thread pending another request to run something on it.
    //											// Presumably the difference is that thread de/allocation is cheaper on Solaris than on SGI...?
    // 
    extern Pthread* pth__get_pthread		(void);					// Needed to find record for current pthread in contexts like signal handlers where it is not (otherwise) available.
    //											// Pthread is typedef'ed in src/c/h/runtime-base.h
    //
    extern Pid      pth__get_pthread_id		(void);					// Used to initialize pthread_table__global[0]->pid in   src/c/main/runtime-state.c
    //											// This just calls getpid()  in                         src/c/pthread/pthread-on-sgi.c
    //											// This returns thr_self() (I don't wanna know) in      src/c/pthread/pthread-on-solaris.c
    //
    extern int      pth__get_active_pthread_count();					// Just returns (as a C int) the value of   ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL, which is defined in   src/c/h/runtime-globals.h
											// Used only to set barrier for right number of pthreads in   src/c/heapcleaner/pthread-heapcleaner-stuff.c


    ////////////////////////////////////////////////////////////////////////////
    // MULTICORE GARBAGE COLLECTION SUPPORT
    //
    extern int   pth__start_heapcleaning    (Task*);
    extern void  pth__finish_heapcleaning   (Task*);
    //
    extern Val*  pth__extra_heapcleaner_roots__global [];



    ////////////////////////////////////////////////////////////////////////////
    //                   MUTEX LOCKS
    //
    // We use our "locks" to perform mutual exclusion,
    // ensuring consistency of shared mutable datastructures
    // by ensuring that at most one pthread at a time is
    // updating that datastructure.  Typically we allocate
    // one such lock for each major shared mutable datastructure,
    // which persists for as long as that datastructure.
    //
    extern void  pth__mutex_init	(Mutex* mutex);				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_init.html
    extern void  pth__mutex_destroy	(Mutex* mutex);				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_init.html
    //
    extern void  pth__mutex_lock	(Mutex* mutex);				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_lock.html
    extern void  pth__mutex_unlock	(Mutex* mutex);				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_lock.html
    extern Bool  pth__mutex_trylock     (Mutex* mutex);				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_lock.html
    //										// pth__mutex_trylock returns FALSE if lock was acquired, TRUE if it was busy.
    //										// This bool value is confusing -- the Mythryl-level binding should return (say) ACQUIRED vs BUSY.
    // Some statically pre-allocated mutexs:
    //
    extern Mutex	    pth__heapcleaner_mutex__global;
    extern Mutex	    pth__heapcleaner_gen_mutex__global;
    extern Mutex	    pth__timer_mutex__global;
    //
    extern Barrier	    pth__heapcleaner_barrier__global;
    //
    //


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
    //     src/c/heapcleaner/pthread-heapcleaner-stuff.c
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
    // NB: This facility seems to be implemented directly in hardware in    src/c/pthread/pthread-on-sgi.c
    // but implemented on top of mutexs in                                  src/c/pthread/pthread-on-solaris.c
    //
    extern void     pth__barrier_init 	(Barrier* barrier, int threads);	// Set up barrier for n-thread wait.
    extern void     pth__free_barrier	(Barrier* barrierp);			// Free a barrier.
    //
    extern void     pth__barrier_wait (Barrier* barrierp);			// Should be called 'barrier_wait' or such.  Block pthread until 'n' pthreads are waiting at the barrier, then release them all.
    //										// It is presumed that all threads waiting on a barrier use the same value of 'n'; otherwise behavior is probably undefined. (Poor design IMHO.)
    //





#endif // NEED_PTHREAD_SUPPORT

#endif // RUNTIME_MULTICORE_H



// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

