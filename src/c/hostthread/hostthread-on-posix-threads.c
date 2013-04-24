// hostthread-on-posix-threads.c
//
// For background see
//
//     src/A.HOSTTHREAD-SUPPORT.OVERVIEW
//
// and the docs at the bottom of
// 
//     src/c/hostthread/hostthread-on-posix-threads.c
//
// This file contains our actual calls directly
// to the <pthread.h> routines.
//
// This code is derived in (small! :-) part from the
// original 1994 sgi-mp.c file from the SML/NJ codebase.



/*
###                "Light is the task when many share the toil."
###
###                                 -- Homer, circa 750BC
*/



#include "../mythryl-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif


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


// Some statically allocated locks.
//
// We try to put each mutex in its own cache line
// to prevent cores thrashing against each other
// trying to get control of logically unrelated mutexs:
//
// It would presumably be good to force cache-line-size
// alignment here, but I don't know how, short of	// LATER: But see synopsis of posix_memalign in src/c/lib/hostthread/libmythryl-hostthread.c
// malloc'ing and checking alignment at runtime:
/**/													char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];

        // We place these two here in the hope that they
	// might wind up in the same cache line as their
	// mutex and condvar (pth__mutex and pth__condvar)
	// because that could somewhat reduce inter-core
	// ram traffic:
	//
        Heapcleaner_State  pth__heapcleaner_state__global	=  HEAPCLEANER_IS_OFF;				// Grab pth__mutex before changing this.  Signal pth__condvar after such changes.
        int                pth__running_hostthreads_count__global  =  1;					// Grab pth__mutex before changing this.  Signal pth__condvar after such changes.
		    //
		    // pth__running_hostthreads_count__global must always equal the number
		    // of hostthreads with hostthread->mode == HOSTTHREAD_IS_RUNNING.

        Mutex	 pth__ramlog_mutex	= PTHREAD_MUTEX_INITIALIZER;
                    //
		    // Used in   src/c/main/ramlog.c

        Mutex	 pth__mutex		= PTHREAD_MUTEX_INITIALIZER;						// No padding here because it might as well share a cache line with next.
                    //
		    // Grab this mutex before changing any of:
                    //
		    //     hostthread->mode fields or global flag
		    //     pth__heapcleaner_state__global
		    //     pth__running_hostthreads_count__global
		    //
		    // This mutex is also used in
		    //
		    //     src/c/heapcleaner/make-strings-and-vectors-etc.c
		    //
		    // to serialize access to the shared (non-agegroup0)
		    // heap agegroups.

        Condvar	 pth__condvar	= PTHREAD_COND_INITIALIZER;						char     pth__cacheline_padding0[ CACHE_LINE_BYTESIZE ];
		    //
		    // Hostthreads block in this condvar while
		    // waiting for some predicate over
                    //
		    //     hostthread->mode fields or global flag
		    //     pth__heapcleaner_state__global
		    //     pth__running_hostthreads_count__global
                    //
		    // to become TRUE.




/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// Dynamically allocated mutexes                     START OF SECTION
//
// Major design considerations and
// constraints here include:
//
//   o We need to have Mythryl-level mutex values in
//         src/lib/std/src/hostthread.pkg
//     It is ok if they are proxies for actual C-level mutexes,
//     so long as Mythryl-level semantics stay clean and natural.
//
//   o We cannot (trivially) allocate C mutexes structs
//     on the Mythryl heap because the Linux mutex struct
//     includes pointers, and is thus position-dependent,
//     whereas Mythryl heap values typically move around.
//
//   o We build the Mythryl compiler (etc) by saving out
//     Mythryl heap images to disk and later reloading them.
//     We would like to have static global mutexes
//     in packages like   src/lib/std/src/io/winix-text-file-for-os-g--premicrothread.pkg
//     so mutexes must be implemented in a way which will
//     survive such heap save/load cycles.
//
//   o We don't want to have any fixed limit on allowed
//     number of mutexes.
//
// Our design solution here:
//
//   o We use Int31 values as Mythryl-level mutex proxies.
//     These are small-int vector indices, much in the spirit
//     of C small-int file handles.
//
//   o Whenever we handle an Int31 mutex proxy value,
//     we create the required mutex if it does not already
//     exist, in order to transparently handle saved-and-reloaded
//     heap images without special logic in the heap save/load code.
//
//   o We maintain a C-level vector of mutexes.
//
//   o We malloc() this vector so that we can realloc()
//     as necessary to expand it.
//
//   o When we expand this vector we double it in size, so as to
//     stay within a factor of two of optimal space and time cost.
//
//   o We keep this vector's length a power of two, so as to
//     facilitate circular traversal via simple masking.
//
//   o We keep unused vector slots set to NULL, and allocate
//     mutexes by simply rotating a clock-hand circularly around
//     the vector looking for NULL slots.
//
//
static Mutex**	mutex_vector__local                       =  NULL;			// This will be allocated in make_mutex_vector(), sized per next.
static Vunt	last_valid_mutex_vector_slot_index__local =  (1 << 12) -1;		// Must be power of two minus one.  We start with large vector to reduce potential race condition problems involving stale ids.
static Vunt	mutex_vector_cursor__local                =  0;				// Rotates circularly around mutex_vector__local


//
static char*   initialize_mutex   (Mutex* mutex) {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_init.html
    //         ================
    //
    int err =  pthread_mutex_init( mutex, NULL );					// pthread_mutex_init probably cannot block, so we probably do not need the RELEASE/RECOVER wrappers.

    switch (err) {
	//
	case 0:				return NULL;					// Success.
	case ENOMEM:			return "Insufficient ram to initialize mutex";
	case EAGAIN:			return "Insufficient (non-ram) resources to initialize mutex";
	case EPERM:			return "Caller lacks privilege to initialize mutex";
	case EBUSY:			return "Attempt to reinitialize the object referenced by mutex, a previously initialized, but not yet destroyed, mutex.";
	case EINVAL:			return "Invalid attribute";
	default:			return "Undocumented error return from pthread_mutex_init()";
    }
}

//
static void   make_mutex_vector   (void) {			// Called by pth__start_up(), below.
    //        =================
    //
											//    "{malloc, calloc, realloc, free, posix_memalign} of glibc-2.2+ are thread safe"
											//
											//	-- http://linux.derkeiler.com/Newsgroups/comp.os.linux.development.apps/2005-07/0323.html
    mutex_vector__local
	=
	(Mutex**) malloc(   (last_valid_mutex_vector_slot_index__local +1) * sizeof (Mutex*)   );

    for (Vunt	u  =  0;
		u <=  last_valid_mutex_vector_slot_index__local;
		u ++
    ){
	mutex_vector__local[ u ] =  NULL;
    }
}


//
static void   double_size_of_mutex_vector__need_mutex   (void)   {	// Caller MUST BE HOLDING pth__mutex.
    //        =======================================
    //
    Vunt  new_size_in_slots =   2 * (last_valid_mutex_vector_slot_index__local + 1);
    //
    mutex_vector__local
	=
	(Mutex**) realloc( mutex_vector__local, new_size_in_slots * sizeof(Mutex*) );
											if (!mutex_vector__local)   die("src/c/hostthread/hostthread-on-posix-threads.c: Unable to expand mutex_vector__local to %d slots", new_size_in_slots );
    for (Vunt	u =  last_valid_mutex_vector_slot_index__local + 1;
		u <  new_size_in_slots;
		u ++
    ){
	mutex_vector__local[ u ] =  NULL;
    }

    last_valid_mutex_vector_slot_index__local
	=
	new_size_in_slots - 1;
}

//
static Mutex*   make_mutex_record   (void) {
    //          =================
    //
    Mutex* mutex =   (Mutex*)  malloc( sizeof( Mutex ) );	if (!mutex)  die("src/c/hostthread/hostthread-on-posix-threads.c: allocate_mutex_record: Unable to allocate mutex record");

    char* err =  initialize_mutex( mutex );			if (err)  die("src/c/hostthread/hostthread-on-posix-threads.c: allocate_mutex_record: %s", err);

    return mutex;
}

//
Mutex*   find_mutex_by_id__need_mutex   (Vunt  id) {							// Caller MUST BE HOLDING pth__mutex.
    //   ============================
    //
    while (id > last_valid_mutex_vector_slot_index__local) {
	//
	double_size_of_mutex_vector__need_mutex ();
    }

    if (mutex_vector__local[ id ] == NULL) {
	mutex_vector__local[ id ] =  make_mutex_record ();						// We do this so that stale mutex ids due to heap save/load sequences will work.
    }

    return mutex_vector__local[ id ];
}

//
Vunt   pth__mutex_make   (void) {									// Create a new mutex, return its slot number in mutex_vector__local[].
    // ===============
    //
    pthread_mutex_lock( &pth__mutex );
    //
    for (;;) {												// If vector is initially full, it will be half-empty after we double its size, so we'll loop at most twice.
	//
	// Search for an empty slot in mutex_vector__local[].
	//
	// We start where last search stopped, to avoid
	// wasting time searching start of vector over and over:
	//
	for (Vunt u  =  0;
		  u <=  last_valid_mutex_vector_slot_index__local;
		  u ++
	){
	    mutex_vector_cursor__local = (mutex_vector_cursor__local +1)				// Bump cursor.
				       & last_valid_mutex_vector_slot_index__local;			// Wrap around at end of vector.

	    if (mutex_vector__local[ mutex_vector_cursor__local ] == NULL) {
		mutex_vector__local[ mutex_vector_cursor__local ] =  make_mutex_record ();		// Found an empty slot -- fill it and return its index.

		Vunt result = mutex_vector_cursor__local;

		pthread_mutex_unlock( &pth__mutex );

		return result;
	    }
	}

	// If we arrive here, there are no
	// empty slots in mutex_vector__local[]
	// so we double its size to create some:
	//
	double_size_of_mutex_vector__need_mutex ();
    }	
}

//
// Dynamically allocated mutexes                       END OF SECTION
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// Dynamically allocated condvars                     START OF SECTION
//
// This is just a clone of the above dynamically-allocated mutex section.
//
//
static Condvar** condvar_vector__local                       =  NULL;			// This will be allocated in make_condvar_vector(), sized per next.
static Vunt	 last_valid_condvar_vector_slot_index__local =  (1 << 12) -1;		// Must be power of two minus one.  We start with large vector to reduce potential race condition problems involving stale ids.
static Vunt	 condvar_vector_cursor__local                =  0;			// Rotates circularly around condvar_vector__local


//
static char*   initialize_condvar   (Condvar* condvar) {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_init.html
    //         ==================
    //
    int err =  pthread_cond_init( condvar, NULL );					// pthread_cond_init probably cannot block, so we probably do not need RELEASE/RECOVER wrappers.

    switch (err) {
	//
	case 0:				return NULL;					// Success.
	case ENOMEM:			return "Insufficient ram to initialize condvar";
	case EAGAIN:			return "Insufficient (non-ram) resources to initialize condvar";
	case EPERM:			return "Caller lacks privilege to initialize condvar";
	case EBUSY:			return "Attempt to reinitialize the object referenced by condvar, a previously initialized, but not yet destroyed, condvar.";
	case EINVAL:			return "Invalid attribute";
	default:			return "Undocumented error return from pthread_cond_init()";
    }
}

//
static void   make_condvar_vector   (void) {						// Called by pth__start_up(), below.
    //        ===================
    //
											//    "{malloc, calloc, realloc, free, posix_memalign} of glibc-2.2+ are thread safe"
											//
											//	-- http://linux.derkeiler.com/Newsgroups/comp.os.linux.development.apps/2005-07/0323.html
    condvar_vector__local
	=
	(Condvar**) malloc(   (last_valid_condvar_vector_slot_index__local +1) * sizeof (Condvar*)   );

    for (Vunt	u = 0;
		u <= last_valid_condvar_vector_slot_index__local;
		u ++
    ){
	//
	condvar_vector__local[ u ] =  NULL;
    }
}


//
static void   double_size_of_condvar_vector__need_mutex   (void)   {			// Caller MUST BE HOLDING pth__mutex.
    //        =========================================
    //
    Vunt  new_size_in_slots =   2 * (last_valid_condvar_vector_slot_index__local + 1);
    //
    condvar_vector__local
	=
	(Condvar**) realloc( condvar_vector__local, new_size_in_slots * sizeof(Condvar*) );
											if (!condvar_vector__local) die("src/c/hostthread/hostthread-on-posix-threads.c: Unable to expand condvar_vector__local to %d slots", new_size_in_slots );
    for (Vunt	u =  last_valid_condvar_vector_slot_index__local + 1;
		u <  new_size_in_slots;
		u ++
    ){
	condvar_vector__local[ u ] =  NULL;
    }

    last_valid_condvar_vector_slot_index__local
	=
	new_size_in_slots - 1;
}

//
static Condvar*   make_condvar_record   (void) {
    //            ===================
    //
    Condvar* condvar =   (Condvar*)  malloc( sizeof( Condvar ) );	if (!condvar)  die("src/c/hostthread/hostthread-on-posix-threads.c: allocate_condvar_record: Unable to allocate condvar record");

    char* err =  initialize_condvar( condvar );				if (err)  die("src/c/hostthread/hostthread-on-posix-threads.c: allocate_condvar_record: %s", err);

    return condvar;
}

//
Condvar*   find_condvar_by_id__need_mutex   (Vunt  id) {				// Caller MUST BE HOLDING pth__mutex.
    //     ==============================
    //
    while (id > last_valid_condvar_vector_slot_index__local) {
	//
	double_size_of_condvar_vector__need_mutex ();
    }

    if (condvar_vector__local[ id ] == NULL) {
	condvar_vector__local[ id ] =  make_condvar_record ();						// We do this so that stale condvar ids due to heap save/load sequences will work.
    }

    return condvar_vector__local[ id ];
}

//
Vunt   pth__condvar_make   (void) {									// Create a new condvar, return its slot number in condvar_vector__local[].
    // =================
    //
    pthread_mutex_lock( &pth__mutex );
    //
    for (;;) {												// If vector is initially full, it will be half-empty after we double its size, so we'll loop at most twice.
	//
	// Search for an empty slot in condvar_vector__local[].
	//
	// We start where last search stopped, to avoid
	// wasting time searching start of vector over and over:
	//
	for (Vunt   u  =  0;
		    u <=  last_valid_condvar_vector_slot_index__local;
		    u ++
	){
	    condvar_vector_cursor__local =  (condvar_vector_cursor__local +1)				// Bump cursor.
				         &  last_valid_condvar_vector_slot_index__local;		// Wrap around at end of vector.

	    if (condvar_vector__local[ condvar_vector_cursor__local ] == NULL) {
		condvar_vector__local[ condvar_vector_cursor__local ] =  make_condvar_record ();	// Found an empty slot -- fill it and return its index.
		Vunt result =          condvar_vector_cursor__local;

		pthread_mutex_unlock( &pth__mutex );

		return result;
	    }
	}

	// If we arrive here, there are no
	// empty slots in condvar_vector__local[]
	// so we double its size to create some:
	//
	double_size_of_condvar_vector__need_mutex ();
    }	
}

//
// Dynamically allocated condvars                       END OF SECTION
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// Dynamically allocated barriers                     START OF SECTION
//
// This is just a clone of the above dynamically-allocated mutex section.
//
//

#if !defined(HAVE_PTHREAD_BARRIER_T)

// We need to emulate pthread_barrier_t since the OS doesn't provide one.
// This was blatantly stolen from here: http://www.howforge.com/implementing-barrier-in-pthreads
//             -- Dave Huseby

int  barrier_init_emulation  (Barrier* barrier,  int needed) {
    //
    barrier->needed = needed;
    barrier->called = 0;
    //NOTE: error checking is for the weak minded
    pthread_mutex_init( &barrier->mutex, NULL );
    pthread_cond_init( &barrier->cond, NULL );
    return 0;
}

int  barrier_destroy_emulation  (Barrier* barrier) {
    //
    pthread_mutex_destroy( &barrier->mutex );
    pthread_cond_destroy( &barrier->cond );
    return 0;
}

int  barrier_wait_emulation  (Barrier* barrier) {
    //
    int ret = 0;
    pthread_mutex_lock( &barrier->mutex );
    barrier->called++;
    if (barrier->called == barrier->needed) {
	//
	barrier->called = 0;
	pthread_cond_broadcast( &barrier->cond );
	ret = PTHREAD_BARRIER_SERIAL_THREAD;

    } else {

	pthread_cond_wait( &barrier->cond, &barrier->mutex );
    }
    pthread_mutex_unlock( &barrier->mutex );
    return ret;
}

#endif


typedef struct { Barrier barrier;
                 Bool    is_set;
               }
        Barrierx; 										// "x" for "extended" (with "is_set" flag).

static Barrierx** barrier_vector__local                       =  NULL;				// This will be allocated in make_barrier_vector(), sized per next.
static Vunt	  last_valid_barrier_vector_slot_index__local =  (1 << 12) -1;			// Must be power of two minus one.  We start with large vector to reduce potential race condition problems involving stale ids.
static Vunt	  barrier_vector_cursor__local                =  0;				// Rotates circularly around barrier_vector__local


//
// static char*   initialize_barrier   (Barrierx* barrier, int threads) {
//     //         ==================
//     //
//     int err =  pthread_barrier_init( &barrier->barrier, NULL, (unsigned) threads );			// pthread_barrier_init probably cannot block, so we probably do not need RELEASE/RECOVER wrappers.
// 
//     switch (err) {
// 	//
// 	case 0:				return NULL;					// Success.
// 	case ENOMEM:			return "Insufficient ram to initialize barrier";
// 	case EAGAIN:			return "Insufficient (non-ram) resources to initialize barrier";
// 	case EPERM:			return "Caller lacks privilege to initialize barrier";
// 	case EBUSY:			return "Attempt to reinitialize the object referenced by barrier, a previously initialized, but not yet destroyed, barrier.";
// 	case EINVAL:			return "Invalid attribute";
// 	default:			return "Undocumented error return from pthread_barrier_init()";
//     }
// }

//
static void   make_barrier_vector   (void) {						// Called by pth__start_up(), below.
    //        ===================
    //
											//    "{malloc, calloc, realloc, free, posix_memalign} of glibc-2.2+ are thread safe"
											//
											//	-- http://linux.derkeiler.com/Newsgroups/comp.os.linux.development.apps/2005-07/0323.html
    barrier_vector__local
	=
	(Barrierx**) malloc(   (last_valid_barrier_vector_slot_index__local +1) * sizeof (Barrierx*)   );

    for (Vunt	u  =  0;
		u <=  last_valid_barrier_vector_slot_index__local;
		u ++
    ){
	//
	barrier_vector__local[ u ] =  NULL;
    }
}


//
static void   double_size_of_barrier_vector__need_mutex   (void)   {			// Caller MUST BE HOLDING pth__mutex.
    //        =========================================
    //
    Vunt  new_size_in_slots =   2 * (last_valid_barrier_vector_slot_index__local + 1);
    //
    barrier_vector__local
	=
	(Barrierx**) realloc( barrier_vector__local, new_size_in_slots * sizeof(Barrierx*) );
											if (!barrier_vector__local) die("src/c/hostthread/hostthread-on-posix-threads.c: Unable to expand barrier_vector__local to %d slots", new_size_in_slots );
    for (Vunt	u =  last_valid_barrier_vector_slot_index__local + 1;
		u <  new_size_in_slots;
		u ++
    ){
	barrier_vector__local[ u ] =  NULL;
    }

    last_valid_barrier_vector_slot_index__local
	=
	new_size_in_slots - 1;
}

//
static Barrierx*  make_barrier_record   (void) {
    //            ===================
    //
    Barrierx* barrier =   (Barrierx*)  malloc( sizeof( Barrierx ) );	if (!barrier)  die("src/c/hostthread/hostthread-on-posix-threads.c: allocate_barrier_record: Unable to allocate barrier record");

    barrier->is_set   = FALSE;

    return barrier;
}

//
Barrierx*   find_barrier_by_id__need_mutex   (Vunt  id) {					// Caller MUST BE HOLDING pth__mutex.
    //      ==============================
    //
    while (id > last_valid_barrier_vector_slot_index__local) {
	//
	double_size_of_barrier_vector__need_mutex ();
    }

    if (barrier_vector__local[ id ] == NULL) {
	barrier_vector__local[ id ] =  make_barrier_record ();					// We do this so that stale barrier ids due to heap save/load sequences will work.
    }

    return barrier_vector__local[ id ];
}

//
Vunt   pth__barrier_make   (void) {									// Create a new barrier, return its slot number in barrier_vector__local[].
    // =================
    //
    pthread_mutex_lock( &pth__mutex );
    //
    for (;;) {												// If vector is initially full, it will be half-empty after we double its size, so we'll loop at most twice.
	//
	// Search for an empty slot in barrier_vector__local[].
	//
	// We start where last search stopped, to avoid
	// wasting time searching start of vector over and over:
	//
	for (Vunt   u  =  0;
		    u <=  last_valid_barrier_vector_slot_index__local;
		    u ++
	){
	    barrier_vector_cursor__local =  (barrier_vector_cursor__local +1)				// Bump cursor.
				         &  last_valid_barrier_vector_slot_index__local;		// Wrap around at end of vector.

	    if (barrier_vector__local[ barrier_vector_cursor__local ] == NULL) {
		barrier_vector__local[ barrier_vector_cursor__local ] =  make_barrier_record ();	// Found an empty slot -- fill it and return its index.
		Vunt result =          barrier_vector_cursor__local;

		pthread_mutex_unlock( &pth__mutex );

		return result;
	    }
	}

	// If we arrive here, there are no
	// empty slots in barrier_vector__local[]
	// so we double its size to create some:
	//
	double_size_of_barrier_vector__need_mutex ();
    }	
}

//
// Dynamically allocated barriers                       END OF SECTION
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////





//
static void*  hostthread_main   (void* task_as_voidptr)   {
    //        ===============
    //
    // This is the top-level function we execute within
    // hostthreads spawned from within Mythryl code:
    // pth__pthread_create() passes us to  pthread_create().
    //
    // Should we maybe be clearing some or all of the signal mask here?
    //
    //     "The signal state of the new thread shall be initialized as follows:
    //          The signal mask shall be inherited from the creating thread.
    //          The set of signals pending for the new thread shall be empty."
    //
    //      -- http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_create.html	


    Task* task = (Task*) task_as_voidptr;								// The <pthread.h> API for pthread_create requires that our arg be cast to void*; cast it back to its real type.


    pthread_mutex_unlock( &pth__mutex );								// This lock was acquired by pth__pthread_create (below).

    run_mythryl_task_and_runtime_eventloop__may_heapclean( task, NULL );				// run_mythryl_task_and_runtime_eventloop__may_heapclean	def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c


    // run_mythryl_task_and_runtime_eventloop__may_heapclean should never return:
    //
    die ("hostthread_main:  Returned fromd run_mythryl_task_and_runtime_eventloop__may_heapclean()?!\n");

    return  (void*) NULL;										// Cannot execute; only to keep gcc quiet.
}




													// typedef   struct task   Task;	def in   src/c/h/runtime-base.h
													// struct task				def in   src/c/h/runtime-base.h

char* pth__pthread_create   (int* hostthread_table_slot, Val current_thread, Val closure_arg)   {	// "It's the job that's never started as takes longest to finish."   -- J. R. R. Tolkein
    //===================
    //
    // Run 'closure_arg' in its own kernel thread.
    //
    // This fn is called (only) by   spawn_hostthread ()   in   src/c/lib/hostthread/libmythryl-hostthread.c
    //
    Task*    task;
    Hostthread* hostthread;
    int      i;

													HOSTTHREAD_LOG_IF ("[Searching for free hostthread]\n");

    pthread_mutex_lock( &pth__mutex );									// Always first step before reading/writing hostthread_table__global.

    // Search for a slot in which to put a new hostthread
    //
    for (i = 0;
	(i < MAX_HOSTTHREADS)  &&  (hostthread_table__global[i]->mode != HOSTTHREAD_IS_VOID);
	i++
    ){
	continue;
    }

    if (i == MAX_HOSTTHREADS) {
	//
	pthread_mutex_unlock( &pth__mutex );
	return  "hostthread_table__global full -- increase MAX_HOSTTHREADS?";
    }

													HOSTTHREAD_LOG_IF ("[using hostthread_table__global slot %d]\n", i);

    // Use hostthread at index i:
    //
    *hostthread_table_slot = i;
    //
    hostthread =  hostthread_table__global[ i ];

    task =  hostthread->task;

    task->exception_fate	=  PTR_CAST( Val,  handle_uncaught_exception_closure_v + 1 );	// Defined by   ASM_CLOSURE(handle_uncaught_exception_closure);   in   src/c/main/construct-runtime-package.c
												// in reference to handle_uncaught_exception_closure_asm	  in   src/c/machine-dependent/prim*asm 
    task->argument		=  HEAP_VOID;
    //
    task->fate			=  PTR_CAST( Val, return_to_c_level_c);				// Defined by   ASM_CONT(return_to_c_level);			  in   src/c/main/construct-runtime-package.c
												// in reference to return_to_c_level_asm            		  in   src/c/machine-dependent/prim*asm 
    task->current_closure	=  closure_arg;
    //
    task->program_counter	= 
    task->link_register		=  GET_CODE_ADDRESS_FROM_CLOSURE( closure_arg );		// GET_CODE_ADDRESS_FROM_CLOSURE				is from   src/c/h/runtime-values.h
    //
    task->current_thread	=  current_thread;
  

    // 2012-11-22 CrT: A turkey of a fix for Turkey Day:
    // alarm_handler in src/lib/src/lib/thread-kit/src/core-thread-kit/microthread-preemptive-scheduler.pkg
    // is failing because it is being called by the bloodybedamned 50Hz SIGALRM
    // in secondary hostthreads (== posix threads).  And we don't have
    // process masks implemented apparently (grepping for sigprocmask shows nada)
    // and that is a whole can of worms in its own right and I'm already up to my
    // ass in alligators at the moment thank you kindly so here I'm just going to
    // tweak pth__pthread_create so that SIGALRM is masked in all secondary hostthreads,
    // and ditto all other signals which I expect application logic to want to handle
    // only in the primary hostthread -- for details on them see signal(7):
    sigset_t                    original_signal_mask;
    sigset_t                    signals_to_block;
    sigemptyset(               &signals_to_block );						// Allocate the signal set to empty. MUST be done first.
    sigaddset(                 &signals_to_block, SIGALRM );					// Add SIGALRM to the set.
    sigaddset(                 &signals_to_block, SIGABRT );					// ...
    sigaddset(                 &signals_to_block, SIGBUS  );					// 
    sigaddset(                 &signals_to_block, SIGCHLD );					// 
    sigaddset(                 &signals_to_block, SIGCONT );					// 
    sigaddset(                 &signals_to_block, SIGHUP  );					// 
    sigaddset(                 &signals_to_block, SIGINT  );					// 
    sigaddset(                 &signals_to_block, SIGIO   );					// == SIGPOLL
    sigaddset(                 &signals_to_block, SIGPIPE );					// 
    sigaddset(                 &signals_to_block, SIGPROF );					// 
    sigaddset(                 &signals_to_block, SIGQUIT );					// 
    sigaddset(                 &signals_to_block, SIGSTOP );					// 
    sigaddset(                 &signals_to_block, SIGTERM );					// 
    sigaddset(                 &signals_to_block, SIGTRAP );					// 
    sigaddset(                 &signals_to_block, SIGTSTP );					// 
    sigaddset(                 &signals_to_block, SIGTTIN );					// 
    sigaddset(                 &signals_to_block, SIGTTOU );					// 
    sigaddset(                 &signals_to_block, SIGUSR1 );					// 
    sigaddset(                 &signals_to_block, SIGUSR2 );					// 
    sigaddset(                 &signals_to_block, SIGVTALRM );					// 
    sigaddset(                 &signals_to_block, SIGXCPU );					// 
    sigaddset(                 &signals_to_block, SIGXFSZ );					// 
    pthread_sigmask( SIG_BLOCK,&signals_to_block, &original_signal_mask );			// Block SIGALRM. We do this before pthread_create() to avoid any window for problems immediately after pthread creation.  Third arg returns previous signal mask.
												// We unblock SIGALRM in current pthread immediately after pthread_create() but leave it blocked in child pthread (which inherits our signal mask).

    hostthread->mode = HOSTTHREAD_IS_RUNNING;							// Moved this above pthread_create() because that seems safer,
    ++pth__running_hostthreads_count__global;							// otherwise child might run arbitrarily long without this being set. -- 2011-11-10 CrT
    pthread_cond_broadcast( &pth__condvar );							// Let other hostthreads know state has changed.

    int err =   pthread_create(
		    //
		    &task->hostthread->ptid,							// RESULT. NB: Passing a pointer directly to task->hostthread->tid ensures that field is always
												//         valid as seen by both parent and child threads, without using spinlocks or such.
												//	   Passing the pointer is safe (only) because 'tid' is of type pthread_t from <pthread.h>
												//	   -- we define field 'tid' as 'Tid' in src/c/h/hostthread.h
												//	   and   typedef pthread_t Tid;   in   src/c/h/runtime-base.h

		    NULL,									// Provision for attributes -- API futureproofing.

		    hostthread_main,  (void*) task						// Function + argument to run in new kernel thread.
		);

    pthread_sigmask( SIG_SETMASK, &original_signal_mask, NULL );				// Return parent pthread to status quo ante.

    if (!err) {											// Successfully spawned new kernel thread.
	//
	return NULL;										// Report success. NB: Child thread (i.e., hostthread_main() above)  will unlock  pth__mutex  for us.

    } else {											// Failed to spawn new kernel thread.

	hostthread->mode = HOSTTHREAD_IS_VOID;							// Note hostthread record (still) has no associated kernel thread.
	--pth__running_hostthreads_count__global;						// Restore running-threads count to its original value, since we failed to start it.

	pthread_mutex_unlock( &pth__mutex );

 if (running_script__global) log_if("pth__pthread_create: BOTTOM (error).");
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
    // Called (only) by   release_hostthread()   in   src/c/lib/hostthread/libmythryl-hostthread.c
    //
											HOSTTHREAD_LOG_IF ("[release_hostthread: suspending]\n");

 if (running_script__global) log_if("pth__pthread_exit/TOP: Calling heapcleaner.");
    call_heapcleaner( task, 1 );							// call_heapcleaner		def in   /src/c/heapcleaner/call-heapcleaner.c
	//
	// I presume this call must be intended to sweep all live
	// values from this thread's private agegroup-0 buffer
	// into the shared agegroup-1 buffer, so that nothing
	// will be lost if re-initializing the agegroup-zero
	// buffer for a new thread.   -- 2011-11-10 CrT


 if (running_script__global) log_if("pth__pthread_exit/MID: Updating state.");
    pthread_mutex_lock(    &pth__mutex );
	//
	task->hostthread->mode = HOSTTHREAD_IS_VOID;
        --pth__running_hostthreads_count__global;
	pthread_cond_broadcast( &pth__condvar );					// Let other hostthreads know state has changed.
	//
    pthread_mutex_unlock(  &pth__mutex );

 if (running_script__global) log_if("pth__pthread_exit/BOTTOM: Done.");
    pthread_exit( NULL );								// "The pthread_exit() function cannot return to its caller."   -- http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_exit.html
}



char*    pth__pthread_join   (Task* task, Val arg) {						// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_join.html
    //   =================
    //
    // Called (only) by   join_hostthread()   in   src/c/lib/hostthread/libmythryl-hostthread.c

    //////////////////////////////////////////////////////////////////////////////
    // POTENTIAL RACE CONDITION		XXX BUGGO FIXME
    //
    // There is actually a fairly nasty race condition here:
    // The 'arg' sub-hostthread could exit, be marked HOSTTHREAD_IS_VOID,
    // and be re-allocated by a new pth__pthread_create
    // -- all before we reach this point.  In that case we'd wind
    // up using the wrong (new) hostthread->tid value in this call.
    //
    // Two unappealing choices:
    //  o Since we do not know that the Mythryl level code will ever
    //    join a given thread, we cannot wait until after the join
    //    to recycle the record.
    //  o If we never re-use Hostthread records at all, we introduce a
    //    memory leak.
    //
    // Unix has similar problems with recycling of pid values, of course,
    // ameliorated by using 64K of pid values before wrapping around.
    // We could do something similar by identifying hostthreads by Int31 values
    // that wrap around and resolving them by linear search of the Hostthread
    // vector/list, rather than simple indexing. (Or a binary lookup if
    // we started having huge numbers of live Hostthreads, say.)
    //
    // For the moment I only intend to allocate a small number of Hostthreads
    // at startup and then stick with them for the length of the run, so
    // I'm going to punt on this problem for the time being. -- 2011-12-05 CrT
    //////////////////////////////////////////////////////////////////////////////

    int hostthread_to_join_id =  TAGGED_INT_TO_C_INT( arg );

    // 'hostthread_to_join_id' should have been returned by
    // pth__pthread_create  (above) and should be an index into
    // hostthread_table__global[], but let's sanity-check it: 
    //
    if (hostthread_to_join_id < 0
    ||  hostthread_to_join_id >= MAX_HOSTTHREADS
    ){
	return "pth__pthread_join: Bogus value for hostthread_to_join_id.";
    }

    Hostthread* hostthread_to_join =  hostthread_table__global[ hostthread_to_join_id ];		// hostthread_table__global	def in   src/c/main/runtime-state.c

											// The following check is incorrect because
											// the subthread might well complete and exit
											// before we get to this point:    (*BLUSH*! -- 2011-12-05 CrT)
											//
											//  if (hostthread_to_join->mode == HOSTTHREAD_IS_VOID)  {
											//	//
											//	return "pth__pthread_join: Bogus value for hostthread-to-join (already-dead thread?)";
											//  }

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );				// Enter BLOCKED mode.
	//
        int err =  pthread_join( hostthread_to_join->ptid, NULL );			// NULL is a void** arg that can return result of joined thread. We ignore it
	//    										// because the typing would be a pain: we'd have to return Exception, probably -- ick!
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );					// Return to RUNNING mode.


    switch (err) {
	//
	case 0:		return NULL;							// Success.
	//		
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
    ASSIGN( MICROTHREAD_SWITCH_LOCK_REFCELL__GLOBAL, TAGGED_INT_FROM_C_INT(0) );	// Documented in   src/c/h/runtime-globals.h

    // malloc() and initialize our
    // dynamic-allocation vectors:
    //
    make_mutex_vector();
    make_condvar_vector();
    make_barrier_vector();
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

										// NB: All the error returns in this file should interpret the error number;
										// I forget the syntax offhand. XXX SUCKO FIXME -- 2011-11-03 CrT
//
// char*    pth__mutex_init   (Task* task, Mutex* mutex) {			// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_init.html
//    //   ===============
//    //
//    return  initialize_mutex( mutex );
// }

//
char*    pth__mutex_destroy   (Task* task, Vunt mutex_id)   {			// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_init.html
    //   ==================
    //
    pthread_mutex_lock(   &pth__mutex  );
	//
	Mutex* mutex =  find_mutex_by_id__need_mutex( mutex_id );
	//
	int err =  pthread_mutex_destroy( mutex );				// pthread_mutex_destroy probably cannot block, so we probably do not need the RELEASE/RECOVER wrappers, but better safe than sorry.
	//
	free( mutex );
	//
	mutex_vector__local[ mutex_id ] = NULL;
	//
    pthread_mutex_unlock(  &pth__mutex  );

    switch (err) {
	//
	case 0:				return NULL;				// Success.
	case EBUSY:			return "pth__mutex_destroy: attempt to destroy the object referenced by mutex while it is locked or referenced (eg, while used in a pthread_cond_timedwait() or pthread_cond_wait()) by a thread.";
	case EINVAL:			return "pth__mutex_destroy: invalid mutex.";
	default:			return "pth__mutex_destroy: Undocumented error return from pthread_mutex_destroy()";
    }
}

//
char*  pth__mutex_lock  (Task* task, Vunt mutex_id) {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_lock.html
    // ===============
    //
    //
    pthread_mutex_lock(   &pth__mutex  );
	//
	Mutex* mutex =  find_mutex_by_id__need_mutex( mutex_id );
	//
    pthread_mutex_unlock(  &pth__mutex  );

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	int err =  pthread_mutex_lock( mutex );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    switch (err) {
	//
	case 0:				return NULL;				// Success.
	case EINVAL:			return "pth__mutex_lock: Invalid mutex or mutex has HOSTTHREAD_PRIO_PROTECT set and calling thread's priority is higher than mutex's priority ceiling.";
	case EBUSY:			return "pth__mutex_lock: Mutex was already locked.";
	case EAGAIN:			return "pth__mutex_lock: Recursive lock limit exceeded.";
	case EDEADLK:			return "pth__mutex_lock: Deadlock, or mutex already owned by thread.";
	default:			return "pth__mutex_lock: Undocumented error return from pthread_mutex_lock()";
    }
}
//
char*  pth__mutex_trylock   (Task* task, Vunt mutex_id, Bool* result) {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_lock.html
    // ==================
    //
    pthread_mutex_lock(   &pth__mutex  );
	//
	Mutex* mutex =  find_mutex_by_id__need_mutex( mutex_id );
	//
    pthread_mutex_unlock(  &pth__mutex  );

    int err =  pthread_mutex_trylock( mutex );							// pthread_mutex_trylock probably cannot block, so we presumably do not need RELEASE/RECOVER wrappers.

    switch (err) {
	//
	case 0: 	*result = FALSE;	return NULL;					// Successfully acquired mutex.
	case EBUSY:	*result = TRUE;		return NULL;					// Mutex was already taken.
	//
	default:				return "pth__mutex_trylock: Error while attempting to test mutex.";
    }
}
//
char*  pth__mutex_unlock   (Task* task, Vunt mutex_id) {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_mutex_lock.html
    // =================
    //
    //
    pthread_mutex_lock(   &pth__mutex  );
	//
	Mutex* mutex =  find_mutex_by_id__need_mutex( mutex_id );
	//
    pthread_mutex_unlock(  &pth__mutex  );

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	int err =  pthread_mutex_unlock( mutex );						// pthread_mutex_unlock probably cannot block, so we probably do not need the RELEASE/RECOVER wrappers, but better safe than sorry.
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    switch (err) {
	//
	case 0: 				return NULL;					// Successfully released lock.
	case EINVAL:				return "pth__mutex_unlock: Mutex has HOSTTHREAD_PRIO_PROTECT set and calling thread's priority is higher than mutex's priority ceiling.";
	case EBUSY:				return "pth__mutex_unlock: The mutex already locked.";
	//
	default:				return "pth__mutex_unlock: Undocumented error returned by pthread_mutex_unlock().";
    }
}



// Here's a little tutorial on posix condition variables:
//
//     http://www.gentoo.org/doc/en/articles/l-posix3.xml
//
// And a bigger and better one:
//
//     http://learning.infocollections.com/ebook%202/Computer/Operating%20Systems/Linux%20&%20UNIX/Unix.Systems.Programming.Second.Edition/0130424110_ch13lev1sec4.html
//
char*    pth__condvar_init (Task* task, Condvar* condvar) {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_init.html
    //   =================
    //
    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	int result = pthread_cond_init( condvar, NULL );					// pthread_cond_init probably cannot block, so we probably do not need the RELEASE/RECOVER wrappers, but better safe than sorry.
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    if (result)	  return "pth__condvar_init: Unable to initialize condition variable.";
    else	  return NULL;
}
//
char*  pth__condvar_destroy (Task* task, Vunt condvar_id) {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_init.html
    // ====================
    //
    pthread_mutex_lock(   &pth__mutex  );
	//
	Condvar* condvar =  find_condvar_by_id__need_mutex( condvar_id );
	//
	int result =  pthread_cond_destroy( condvar );						// pthread_cond_destroy probably cannot block, so we probably do not need the RELEASE/RECOVER wrappers.
	//
	free( condvar );
	//
	condvar_vector__local[ condvar_id ] = NULL;
	//
    pthread_mutex_unlock(  &pth__mutex  );

    if (result)	  return "pth__condvar_destroy: Unable to destroy condition variable.";
    else	  return NULL;
}
//
char*  pth__condvar_wait   (Task* task, Vunt condvar_id, Vunt mutex_id) {			// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_wait.html
    // =================
    //
    pthread_mutex_lock(   &pth__mutex  );
	//
	Condvar* condvar =  find_condvar_by_id__need_mutex( condvar_id );
	Mutex*   mutex   =  find_mutex_by_id__need_mutex(   mutex_id   );
	//
    pthread_mutex_unlock(  &pth__mutex  );

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	int result = pthread_cond_wait( condvar, mutex );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    if (result)   return "pth__condvar_wait: Unable to wait on condition variable.";
    else	  return NULL;
}
//
char*  pth__condvar_signal   (Task* task, Vunt condvar_id) {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_signal.html
    // ===================
    //
    pthread_mutex_lock(   &pth__mutex  );
	//
	Condvar* condvar =  find_condvar_by_id__need_mutex( condvar_id );
	//
    pthread_mutex_unlock(  &pth__mutex  );

    int result = pthread_cond_signal( condvar );						// pthread_cond_signal probably cannot block, so we probably do not need RELEASE/RECOVER wrappers.

    if (result)		return "pth__condvar_signal: Unable to signal on condition variable.";
    else		return NULL;
}
//
char*  pth__condvar_broadcast   (Task* task, Vunt condvar_id) {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_cond_signal.html
    // ======================
    //
    pthread_mutex_lock(   &pth__mutex  );
	//
	Condvar* condvar =  find_condvar_by_id__need_mutex( condvar_id );
	//
    pthread_mutex_unlock(  &pth__mutex  );

    int result = pthread_cond_broadcast( condvar );						// pthread_cond_broadcast probably cannot block, so we probably do not need RELEASE/RECOVER wrappers.

    if (result)	  return "pth__condvar_broadcast: Unable to broadcast on condition variable.";
    else	  return NULL;
}



//
char*  pth__barrier_init   (Task* task, Vunt barrier_id, int threads) {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_barrier_init.html
    // =================
    //
    pthread_mutex_lock(   &pth__mutex  );
	//
	Barrierx* barrier =  find_barrier_by_id__need_mutex( barrier_id );
	//
    pthread_mutex_unlock(  &pth__mutex  );

    int result = pthread_barrier_init( &barrier->barrier, NULL, (unsigned) threads);		// pthread_barrier_init probably cannot block, so we probably do not need RELEASE/RECOVER wrappers.

    if (result)	  return "pth__barrier_init: Unable to set barrier.";

    barrier->is_set = TRUE;

    return NULL;
}
//
static char*  barrier_destroy   (Task* task, Barrierx* barrier) {				// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_barrier_init.html
    //        ===============
    //
    int result = hostthread_barrier_destroy( &barrier->barrier );					// hostthread_barrier_destroy probably cannot block, so we probably do not need RELEASE/RECOVER wrappers.

    if (result)   return "pth__barrier_destroy: Unable to clear barrier.";

    barrier->is_set = FALSE;

    return NULL;
}

// static char*    barrier_destroy(Task* task, Barrierx* barrier);
    //
    // Undo the effects of   pth__barrier_init ()   on the barrier.
    // ("Destroy" is poor nomenclature; "reset" would be better.)
    //
    //  o Calling pth__barrier_destroy() immediately after a
    //    pth__barrier_wait() call is safe and typical.
    //    To ensure it is done exactly once, it is convenient
    //    to call pth__barrier_destroy() iff pth__barrier_wait()
    //    returns TRUE.
    //
    //  o Behavior is undefined if pth__barrier_destroy()
    //    is called on an uninitialized barrier.
    //    (In particular, behavior is undefined if
    //    pth__barrier_destroy() is called twice in a
    //    row on a barrier.)
    //
    //  o Behavior is undefined if pth__barrier_destroy()
    //    is called when a hostthread is blocked on the barrier.

//
char*  pth__barrier_free   (Task* task, Vunt barrier_id) {					// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_barrier_init.html
    // =================
    //
    pthread_mutex_lock(   &pth__mutex  );
	//
	Barrierx* barrier =  find_barrier_by_id__need_mutex( barrier_id );

        if (barrier->is_set) {
	    //
	    pthread_mutex_unlock(  &pth__mutex  );
	    //
	    return "pth__barrier_wait:  Cannot free barrier while it is set.";
	}
	
	free( barrier );

	barrier_vector__local[ barrier_id ] = NULL;
	//
    pthread_mutex_unlock(  &pth__mutex  );

    return NULL;
}

//
char*  pth__barrier_wait   (Task* task, Vunt barrier_id, Bool* result) {			// http://pubs.opengroup.org/onlinepubs/007904975/functions/pthread_barrier_wait.html
    // =================
    //

    *result = FALSE;										// Make sure a well-defined value is always returned.

    pthread_mutex_lock(   &pth__mutex  );
	//
	Barrierx* barrier =  find_barrier_by_id__need_mutex( barrier_id );
	//
    pthread_mutex_unlock(  &pth__mutex  );

    if (!barrier->is_set)  return "pth__barrier_wait:  Must set barrier before waiting on it.";

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	int err =  pthread_barrier_wait( &barrier->barrier );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    switch (err) {
	//
	case PTHREAD_BARRIER_SERIAL_THREAD:	*result = TRUE;		return barrier_destroy(task, barrier );					// Exactly one hostthread gets TRUE result when released from barrier.
	case 0:								return NULL;								// All other threads at barrier get this.
	default:							return "pth__barrier_wait: Fatal error while blocked at barrier.";
    }
}


//
Ptid   pth__get_hostthread_ptid   ()   {
    // ========================
    //
    // Return a unique value distinguishing
    // the currently running hostthread from all other
    // hostthreads.  This is an 'unsigned long' on
    // Linux but a pointer on OpenBSD 5.0, so
    // we need to be type-agnostic hereabouts:
    //
    // This is needed to find record for current hostthread in contexts
    // like signal handlers where it is not (otherwise) available.
    //
    return  pthread_self();
}

//
Hostthread*  pth__get_hostthread_by_id (int id)   {
    //       =========================
    //
    // Return Hostthread* for given id:
    //
    for (int i = 0;  i < MAX_HOSTTHREADS;  ++i) {
	//
	if (hostthread_table__global[i]->id == id) {
	    return hostthread_table__global[ i ];								// hostthread_table__global	def in   src/c/main/runtime-state.c
	}
    }														// hostthread_table__global exported via    src/c/h/runtime-base.h
    die ("pth__get_hostthread:  id %d not found in hostthread_table__global?!", id);				// 
    return NULL;												// Cannot execute; only to quiet gcc.
}

//
Hostthread*  pth__get_hostthread_by_ptid (Ptid ptid)   {
    //       ===========================
    //
    // Return Hostthread* for given hostthread:
    //
    for (int i = 0;  i < MAX_HOSTTHREADS;  ++i) {
	//
	if (hostthread_table__global[i]->ptid == ptid) {
	    return hostthread_table__global[ i ];								// hostthread_table__global	def in   src/c/main/runtime-state.c
	}
    }														// hostthread_table__global exported via    src/c/h/runtime-base.h
    die ("pth__get_hostthread:  tid %lx not found in hostthread_table__global?!", (unsigned long int) ptid);	// NB: 'ptid' can be an int or pointer type depending on OS. (E.g., unsigned long on Linux, pointer on OpenBSD.)
    return NULL;												// Cannot execute; only to quiet gcc.
}

//
int    pth__get_hostthread_id   ()   {
    // ======================
    //
    // Return a small in uniquely distinguishing
    // the currently running hostthread from all other
    // hostthreads.
    //
    Hostthread* hostthread =  pth__get_hostthread_by_ptid( pth__get_hostthread_ptid() );
    return      hostthread->id;
}



///////////////////////////////////////////////////////////////////////////////////////////
//
// This implements the RELEASE_MYTHRYL_HEAP macro in
//
//     src/c/h/runtime-base.h
//
void   release_mythryl_heap   (Hostthread* hostthread,  const char* fn_name,  Val* arg)   {
    // ====================
    //
    pthread_mutex_lock(  &pth__mutex  );
	//
	hostthread->mode = HOSTTHREAD_IS_BLOCKED;						// Remove us from the set of RUNNING hostthreads.
	--pth__running_hostthreads_count__global;
	//
	if (arg)   hostthread->task->protected_c_arg = arg;				// Protect 'arg' from the heapcleaner by making it a heapcleaner root.
	//										// This is seldom used; one use is in src/c/lib/posix-io/readbuf.c
	//										//                  Another use is in src/c/lib/posix-os/select.c
	//										//		    Another use is in src/c/lib/posix-passwd/getpwuid.c
	//										//		    Another use is in src/c/lib/posix-process-environment/setgid.c
	//										//		    Another use is in src/c/lib/posix-process-environment/setuid.c
	//										//		    Another use is in src/c/lib/socket/recvbuf.c
	//										//		    Another use is in src/c/lib/socket/recvbuffrom.c	
	//										//		    Another use is in src/c/lib/socket/sendbufto.c
	pthread_cond_broadcast( &pth__condvar );					// Tell other hostthreads that shared state has changed.
	//
    pthread_mutex_unlock(  &pth__mutex  );
}


// This implements the RECOVER_MYTHRYL_HEAP macro in
//
//     src/c/h/runtime-base.h
//
void   recover_mythryl_heap   (Hostthread* hostthread,  const char* fn_name) {
    // ====================
    //
    //
    pthread_mutex_lock(   &pth__mutex  );
	//
	while (pth__heapcleaner_state__global != HEAPCLEANER_IS_OFF) {				// Don't re-enter RUNNING mode while a heapcleaning is in progress.
	    //
	    pthread_cond_wait( &pth__condvar, &pth__mutex );
	}
	//
	hostthread->mode = HOSTTHREAD_IS_RUNNING;						// Return us to the set of RUNNING hostthreads.
	++pth__running_hostthreads_count__global;
	//
	hostthread->task->protected_c_arg = &hostthread->task->heapvoid;			// Make 'arg' no longer be a heapcleaner root.
	//
	pthread_cond_broadcast( &pth__condvar );						// Tell other hostthreads that shared state has changed.
	//
    pthread_mutex_unlock(   &pth__mutex   );
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  SPINLOCK CAUTION
//
// http://stackoverflow.com/questions/6603404/when-is-hostthread-spin-lock-the-right-thing-to-use-over-e-g-a-hostthread-mutex
//
// Q: Given that hostthread_spin_lock is available, when would I use it,
//    and when should one not use them?  I.e. how would I decide to
//    protect some shared data structure with either a hostthread mutex
//    or a hostthread spinlock ?
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
//      What happens when the thread that holds the lock gets
//      interrupted by the scheduler before it can release the lock.
//
//   This is of course rare, but it can happen even if the lock is just held
//   for a single variable-increment operation or something else equally trivial.
//
//   In this case, any other threads attempting to obtain the lock will
//   keep spinning until the thread that holds the lock gets scheduled and
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
// to support one hostthread executing threadkit ("Concurrent ML")
// code at full speed while other hostthreads offload I/O-bound
// and CPU-bound processing.
//
// The central problems are
//
//   1) How to maintain Mythryl heap coherency in the
//      face of multiple hostthreads executing Mythryl
//      code in parallel while retaining good performance.
//
//   2) How to maintain Mythryl heap coherency during
//      heapcleaning ("garbage collection").
//
// The matching solutions Morrisett and Tolmach adopted were:
//
//   1) Most heap allocation is done in agegroup zero;
//      by giving each hostthread its own independent
//      agegroup zero, each can allocate at full speed
//      without locking overhead.
//
//      Allocation is also done directly into later heap
//      agegroups, but this happens too seldom to be
//      performance critical, so conventional mutex locking
//      can be used without problem.
//      
//   2) Parallel heapcleaning ("garbage collection") is nontrivial --
//      see Cheng's 2001 CMU thesis: http://mythryl.org/pub/pml/
//      -- so for ease of implementation (and debugging!) they
//      instead used the  pre-existing garbage collector, running
//      on a single hostthread.
//
//
// Point (2) requires further analysis and design effort.
//
// The central problem is that (with the existing algorithm)
// heapcleaning cannot be done while Mythryl hostthreads are
// reading or writing the Mythryl heap.  Consequently before
// heapcleaning can begin, all hostthreads must be signalled to
// suspend use of the Mythryl heap and must be confirmed to
// have done so.
//
// Furthermore, each hostthread must be brought to a halt with
// its private agegroup-zero buffer in a consistent state
// intelligible to the heapcleaner; in particular there can
// be no allocated but uninitialized pointers in the heap
// containing nonsense values which might make the heapcleaner
// segfault.
//
// The Mythryl compiler guarantees that all Mythryl code
// frequently runs the out-of-heap-space probe logic -- in
// particular that every closed loop through the code contains
// at least one such probe call.  It also guarantees that such
// probes calls are done only at the start of execution of
// a function, when the heap is in a self-consistent state.
// It is thus simple and natural to take advantage of this
// mechanism by having these heap-limit probe calls check a
// global 'pth__heapcleaner_state__global' statevar and (if it is not
// HEAPCLEANER_IS_OFF) suspend execution of Mythryl code and
// enter a special "heapcleaning mode".
// 
// This solves half our problem:  When it is time to do a
// heapcleaning, we set
//     pth__heapcleaner_state__global = HEAPCLEANER_IS_STARTING
// and wait for all running Mythryl hostthreads to stop running
// and enter heapcleaning mode.
//
// The remaining half of the problem is that some hostthreads
// may be blocked on a system call like sleep() or select()
// and might not notice that
//     pth__heapcleaner_state__global
// has been set to HEAPCLEANER_IS_STARTING for milliseconds --
// or even seconds, minutes or hours.
// We run the heapcleaner about 200 times per second, and
// it cannot start until all hostthreads are known not to be
// reading or writing the Mythryl heap, so waiting for all
// system calls to return is out of the question.
//
// Our solution is to distinquish three different hostthread 'modes':
//
//    HOSTTHREAD_IS_RUNNING:    Hostthread is actively running Mythryl code
//                   and reading and writing the Mythryl heap;
//                   it can be counted upon to respond quickly if
//                   pth__heapcleaner_state__global != HEAPCLEANER_IS_OFF
//                   (where "quickly" means microseconds not
//                   milliseconds).
//
//    HOSTTHREAD_IS_BLOCKED:    Hostthread may be blocked indefinitely in
//                   a sleep() or select() or such and thus
//                   cannot be counted upon to respond quickly if
//                   pth__heapcleaner_state__global != HEAPCLEANER_IS_OFF
//                   but it is guaranteed not to be reading
//                   or writing the Mythryl heap, or to
//                   contain any hidden pointers into the
//                   Mythryl heap, so heapcleaning can proceed
//                   safely.
//
//    HOSTTHREAD_IS_PRIMARY_HEAPCLEANER:
//    HOSTTHREAD_IS_SECONDARY_HEAPCLEANER:
//		     Hostthread has suspended execution for the duration
//		     of a heapcleaning.  The initiating hostthread goes to state
//                   HOSTTHREAD_IS_PRIMARY_HEAPCLEANER and does all the actual work;
//                   the remaining RUNNING hostthreads go to state
//                   HOSTTHREAD_IS_SECONDARY_HEAPCLEANER and do no actual work.
//                   
// 
// The idea is then that when heapcleaning is necessary we
// 
//   1) Set pth__heapcleaner_state__global to HEAPCLEANER_IS_STARTING
//      and prevent BLOCKED hostthreads from entering RUNNING mode.
// 
//   2) Wait until the number of hostthreads with
//      hostthread->mode ==  HOSTTHREAD_IS_RUNNING drops to zero.
// 
//   3) Set pth__heapcleaner_state__global to HEAPCLEANER_IS_RUNNING.
//
//   4) Clean the heap. (This may result in some or all
//      records on the heap moving to new addresses.)
// 
//   5) Set pth__heapcleaner_state__global to HEAPCLEANER_IS_OFF,
//      allow all HOSTTHREAD_IS_SECONDARY_HEAPCLEANER hostthreads
//      to return to HOSTTHREAD_IS_RUNNING mode and all
//      HOSTTHREAD_IS_BLOCKED hostthreads to re-enter
//      HOSTTHREAD_IS_RUNNING if they wish.
//
//
//
// To implement this high-level plan we introduce the following
// detailed mechanisms and policies:
//
//
//   o  To distinguish hostthread modes we introduce a type
//
//         Hostthread_Mode = HOSTTHREAD_IS_RUNNING		// Hostthread is running Mythryl code -- will respond quickly to 
//                         | HOSTTHREAD_IS_BLOCKED
//                         | HOSTTHREAD_IS_PRIMARY_HEAPCLEANER
//                         | HOSTTHREAD_IS_SECONDARY_HEAPCLEANER
//			   ;
//      in   src/c/h/runtime-base.h
//
//
//   o  To record our per-thread state we introduce a field
//
//	    hostthread->mode
//
//      of type Hostthread_Mode in the
//      hostthread def in   src/c/h/runtime-base.h
//      
//
//   o  To signal RUNNING hostthreads to enter HEAPCLEANING
//      mode we introduce a Heapcleaner_State enum
//
//          pth__heapcleaner_state__global;
//
//      in   src/c/hostthread/hostthread-on-posix-threads.c
//
// ==>   We set this statevar to HEAPCLEANER_IS_STARTING in   src/c/heapcleaner/hostthread-heapcleaner-stuff.c
//      when we want all HOSTTHREAD_IS_RUNNING hostthreads to switch to HOSTTHREAD_IS_SECONDARY_HEAPCLEANER mode;
//
//      Function   need_to_call_heapcleaner() in   src/c/heapcleaner/call-heapcleaner.c
//      checks this and returns TRUE immediately if it is set.
//
//
//   o  We introduce an int
//
//          pth__running_hostthreads_count__global
//
//      in   src/c/hostthread/hostthread-on-posix-threads.c
//      which is always equal to the number of hostthreads with
//      hostthread->mode == HOSTTHREAD_IS_RUNNING.
//      Heapcleaning cannot begin until this count reaches zero, 
//
//
//   o  We introduce a Mutex
//
//          pth__mutex
//
//      in   src/c/hostthread/hostthread-on-posix-threads.c
//      to govern
//
//           hostthread->mode
//           pth__heapcleaner_state__global
//           pth__running_hostthreads_count__global
//
//      This mutex must be held when making any change to
//      these state variables --  this includes in particular
//      creating or destroying hostthreads.  (It is usually also
//      safest hold this mutex when reading those variables.)
//
//
//   o  We introduce a matching condition variable
//
//          pth__condvar
//
//      in   src/c/hostthread/hostthread-on-posix-threads.c
//      which may be used to wait for a particular state.
//
//
//   o  We introduce a pair of macros
//
//          RELEASE_MYTHRYL_HEAP( hostthread, __func__, &arg );						// Use 'NULL' instead of '&arg' if 'arg' (and all its components) is dead at that point.
//              //
//              syscall();
//              //
//          RECOVER_MYTHRYL_HEAP( hostthread, __func__      );
//
//      in   src/c/h/runtime-base.h
//      to serve as brackets around every system call.
//      
//      (Here 'arg' is the Val argument passed from Mythryl
//      to the C function making the system call -- 'arg'
//      generally needs to be protected against garbage collection.)
//      
//      These macros do:
//      
//          RELEASE_MYTHRYL_HEAP:
//		pthread_mutex_lock(  &pth__mutex  );
//		    hostthread->mode = HOSTTHREAD_IS_BLOCKED;							// Remove us from the set of RUNNING hostthreads.
//		    --pth__running_hostthreads_count__global;
//		    hostthread->task->protected_c_arg = &(arg);						// Protect 'arg' from the heapcleaner by making it a heapcleaner root.
//		    pthread_cond_broadcast( &pth__condvar );						// Let other hostthreads know state has changed.
//		pthread_mutex_unlock(  &pth__mutex  );
//      
//          RECOVER_MYTHRYL_HEAP:
//		pthread_mutex_lock(   &pth__mutex  );
//		    while (pth__heapcleaner_state__global != HEAPCLEANER_IS_OFF) {				// Don't re-enter RUNNING mode while heapcleaner is running.
//			pthread_cond_wait(&pth__condvar,&pth__mutex);
//		    }
//                  hostthread->mode = HOSTTHREAD_IS_RUNNING;							// Return us to the set of RUNNING hostthreads.
//                  ++pth__running_hostthreads_count__global;
//                  hostthread->task->protected_c_arg = &hostthread->task->heapvoid;				// Make 'arg' no longer a heapcleaner root.
//		    pthread_cond_broadcast( &pth__condvar );						// Let other hostthreads know state has changed.
//		pthread_mutex_unlock(   &pth__mutex   );
//
//
//   o  The heapcleaning-initiation logic is then:
//
//	    pthread_mutex_lock(   &pth__mutex  );							// 
//		if (pth__heapcleaner_state__global != HEAPCLEANER_IS_OFF) {
//		    ////////////////////////////////////////////////////////////
//		    // We're a secondary heapcleaner -- we'll just wait() while
//		    // the primary heapcleaner hostthread does all the actual work:
//		    ////////////////////////////////////////////////////////////
//		    hostthread->mode = HOSTTHREAD_IS_SECONDARY_HEAPCLEANER;					// Change from RUNNING to HEAPCLENING mode.
//		    --pth__running_hostthreads_count__global;							// Increment count of HOSTTHREAD_IS_RUNNING mode hostthreads.
//		    pthread_cond_broadcast( &pth__condvar );						// Let other hostthreads know state has changed.
//		    while (pth__heapcleaner_state__global != HEAPCLEANER_IS_OFF) {				// Wait for heapcleaning to complete.
//			pthread_cond_wait(&pth__condvar,&pth__mutex);
//		    }
//		    hostthread->mode = HOSTTHREAD_IS_RUNNING;							// Return to RUNNING mode from SECONDARY_HEAPCLEANER mode.
//		    ++pth__running_hostthreads_count__global;
//		    pthread_cond_broadcast( &pth__condvar );						// Let other hostthreads know state has changed.
//		    pthread_mutex_unlock(  &pth__mutex  );
//		    return FALSE;									// Resume running user code.
//		}
//		/////////////////////////////////////////////////////////////
//		// We're the primary heapcleaner -- we'll do the actual work:
//		/////////////////////////////////////////////////////////////
//		pth__heapcleaner_state__global = HEAPCLEANER_IS_STARTING;					// Signal all HOSTTHREAD_IS_RUNNING hostthreads to block in HOSTTHREAD_IS_SECONDARY_HEAPCLEANER mode
//													// until we set pth__heapcleaner_state__global back to HEAPCLEANER_IS_OFF.
//		hostthread->mode = HOSTTHREAD_IS_PRIMARY_HEAPCLEANER;						// Remove ourself from the set of HOSTTHREAD_IS_RUNNING hostthreads.
//		--pth__running_hostthreads_count__global;
//		pthread_cond_broadcast( &pth__condvar );						// Let other hostthreads know state has changed.
//		while (pth__running_hostthreads_count__global > 0) {						// Wait until all HOSTTHREAD_IS_RUNNING hostthreads have entered HOSTTHREAD_IS_SECONDARY_HEAPCLEANER mode.
//		    pthread_cond_wait( &pth__condvar, &pth__mutex);
//		}
//		pth__heapcleaner_state__global = HEAPCLEANER_IS_RUNNING;					// Note that actual heapcleaning has commenced. This is pure documentation -- nothing tests for this state.
//		pthread_cond_broadcast( &pth__condvar );						// Let other hostthreads know state has changed. (They don't care, but imho it is a good habit to signal each state change.)
//	    pthread_mutex_unlock(  &pth__mutex  );							// Not logically required, but holding a mutex for a long time is a bad habit.
//	    return TRUE;										// Return and run heapcleaner code.
//
//      NB: When our caller completes heapcleaning, it must call
//          pth__finish_heapcleaning()									// pth__finish_heapcleaning	def in   src/c/heapcleaner/hostthread-heapcleaner-stuff.c
//	which needs to unblock the waiting HOSTTHREAD_IS_SECONDARY_HEAPCLEANER
//	and HOSTTHREAD_IS_BLOCKED hostthreads by doing:
//
//	    pthread_mutex_lock(   &pth__mutex  );							// 
//		pth__heapcleaner_state__global = HEAPCLEANER_IS_OFF;						// Clear the enter-heapcleaning-mode signal.
//		hostthread->mode = HOSTTHREAD_IS_RUNNING;							// Return to RUNNING mode from PRIMARY_HEAPCLEANER mode.
//		++pth__running_hostthreads_count__global;								//
//		pthread_cond_broadcast( &pth__condvar );						// Let other hostthreads know state has changed.
//	    pthread_mutex_unlock(  &pth__mutex  );
//
//
//
// NB: We never actually check the hostthread->mode values;
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
//     src/c/lib/hostthread/libmythryl-hostthread.c		Our Mythryl<->C world interface logic.
//
//     src/c/hostthread/hostthread-on-posix-threads.c          Our interface to the <ptheads.h> library proper.
//
//     src/c/h/runtime-base.h                            Contains our API for the previous file.
//
//     src/c/heapcleaner/hostthread-heapcleaner-stuff.c	Added logic to stop all posix threads before starting
//							garbage collection and restart them after it is complete.
//
//     src/c/heapcleaner/call-heapcleaner.c		(Pre-existing file): Tweaks to invoke previous file and
//							to cope with having garbage collector roots in multiple
//							posix threads instead of just one.
//
//     src/c/mythryl-config.h				Critical configuration stuff, for example MAX_HOSTTHREADS.
//
//     src/lib/std/src/hostthread.api			Mythryl-programmer interface to posix-threads functionality.
//     src/lib/std/src/hostthread.pkg			Implementation of previous; this is just wrappers for the calls
//							exported by src/c/lib/hostthread/libmythryl-hostthread.c

///////////////////////////////////////////////////////////////////////
// Note[1]
//
// For awhile we had a   pth__done_pthread_create  bool.
// The idea was that this flag would start out FALSE
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
// This makes no sense to me, but since the mutexes
// are clearly not a performance issue, I eliminated
// the flag to simplify the code.                      -- 2011-11-29 CrT
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
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.





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


