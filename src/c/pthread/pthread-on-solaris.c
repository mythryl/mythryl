// pthread-on-solaris.c
//
// This is an ancient (circa 1994?) implementation of pthread on top
// of the solaris operating system of the era.  These days Solaris
// should have a standard posix-threads implementation, so this file
// can probably be deleted by and by.
//
// Multicore (well, multiprocessor) support for Sparc32 multiprocessor machines running Solaris 2.5
//
// Solaris implementation of externals defined in $(INCLUDE)/runtime-pthread.h


#include "../config.h"

#include <stdio.h>
#include <sys/mman.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>
#include <thread.h>
#include <synch.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <sys/processor.h>
#include <sys/procset.h>
#include "runtime-configuration.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "heap-tags.h"
#include "runtime-pthread.h"
#include "task.h"
#include "runtime-globals.h"
#include "pthread-state.h"


#define INT_LIB7inc(n,i)  ((Val)TAGGED_INT_FROM_C_INT(TAGGED_INT_TO_C_INT(n) + (i)))
#define INT_LIB7dec(n,i)  (INT_LIB7inc(n,(-i)))

 static Lock 	 allocate_lock	();
 static Barrier* allocate_barrier	();
 static void*    allocate_arena_ram	(int size);
 static void     free_arena_ram		(void*, int);
 static void*    pthread_main		(void* task);
 static void*    resume_pthread		(void* vtask);
 static void     suspend_pthread	(Task* task);
 static Task**   initialize_task_vector	();
 static void     bind_to_kernel_thread	(processorid_t*);

static caddr_t          arena_local;				// Arena for shared sync chunks.
static Lock        arena_lock_local;				// Must be held to alloc/free a lock.
static Lock   mp_pthread_lock_local;				// Must be used to acquire/release procs.
static Task**           tasks_local; /*[MAX_PTHREADS]*/		// List of states of suspended pthreads.

#if defined(MP_PROFILE)
    static int *doProfile;
#endif

#define LEAST_PROCESSOR_ID       0
#define GREATEST_PROCESSOR_ID    3

#define NextProcessorId(id)  (((id) == GREATEST_PROCESSOR_ID) ? LEAST_PROCESSOR_ID : (id) + 1)

static processorid_t* processorId;		// processor id of the next processor a lwp will be bound to globals.

Lock	 pth_cleaner_lock_global;
Lock	 pth_cleaner_gen_lock_global;
Lock	 pth_timer_lock_global;
Barrier* pth_cleaner_barrier_global;

#if defined(MP_PROFILE)
    int mutex_trylock_calls;
    int trylock_calls;
#endif



void   pth_initialize   ()   {
    // =============
    //
    // Called (only) from   src/c/main/runtime-main.c

    int fd;

    if ((fd = open("/dev/zero",O_RDWR)) == -1)   die("pth_initialize:Couldn't open /dev/zero");

    arena_local = mmap((caddr_t) 0, sysconf(_SC_PAGESIZE),PROT_READ | PROT_WRITE ,MAP_PRIVATE,fd,0);

    arena_lock_local		= allocate_lock();
    mp_pthread_lock_local	= allocate_lock();
    pth_cleaner_lock_global	= allocate_lock();
    pth_cleaner_gen_lock_global	= allocate_lock();
    pth_timer_lock_global	= allocate_lock();
    pth_cleaner_barrier_global	= allocate_barrier(); 
    tasks_local			= initialize_task_vector();
    //
    ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL, TAGGED_INT_FROM_C_INT(1) );

    #ifdef MP_NONBLOCKING_IO
        MP_InitStdInReader  ();
    #endif

    processorId = (processorid_t *) allocate_arena_ram(sizeof(processorid_t));
   *processorId = -1;

    bind_to_kernel_thread(processorId);

    #ifdef MP_PROFILE
        doProfile = (int *) allocate_arena_ram(sizeof(int));
       *doProfile = 0;
    #endif

    NextProcessorId( *processorId );

// thr_setconcurrency(MAX_PTHREADS);
}


//
static Task**   initialize_task_vector   ()   {
    //          ======================
    //
    // Initialize the array of pointers to ml states of suspended processors.
    // Return the initialized array as a pointer to pointers.


    Task** array = (Task**) allocate_arena_ram(sizeof(Task*));

    for (int i = 1;  i < MAX_PTHREADS;  i++) {
	//
	Task** 	ptr =  (Task**) allocate_arena_ram(sizeof(Task*));

	*ptr = (Task*) NULL;

	ptr++;
    }

    return  array;
}

//
static Lock    allocate_lock   ()   {
    //	       =============
    //	
    // Allocate a portion of the arena of synch chunks for a spin lock.
    // Return a pointer to the allocated region.
    // Created: 5-14-96 	   

    Lock  lock = (Lock) allocate_arena_ram(MP_LOCK_SZ);

    lock->value = UNSET;

    if (mutex_init(&lock->mutex, USYNC_THREAD, NULL) == -1)      die("allocate_lock: unable to initialize mutex");

    return lock;
}


//
static void   free_lock   (Lock lock)   {
    //        =========
    //
    // Destroy the mutex. In addition, if the lock was the last chunk 
    // allocated in the arena then recapture the space occupied by the 
    // lock. Otherwise, zero out the space occupied by the lock.
    // Created 5-14-96

    #if defined(MP_LOCK_DEBUG)
        printf("arena = %ld\t lock = %ld\n",(int) arena_local, lock);
    #endif
 
    mutex_destroy(&lock->mutex);

    free_arena_ram(lock,MP_LOCK_SZ);
}

//
void   bind_to_kernel_thread   (processorid_t* processorId)  {
    // =====================
    //
    // Bind the current lwp to a real processor. Attempt to bind the
    // lwp to a processor different from the last processor a lwp was 
    // bound to.
    // Created 7-22-96 	

    processorid_t procId = *processorId;
    processorid_t obind;
    int lwpBoundP = 0;

    while (!lwpBoundP) {
	//
	procId = NextProcessorId(procId);

	if (procId == *processorId) { 	// Attempts made to bind on all processors.
	    //
	    fprintf(stderr, "lwp was not bound to a processor.\n");
	    lwpBoundP = 1;

	} else {

	    if (processor_bind(P_LWPID, P_MYID, procId, &obind) == -1) {
		//
		fprintf(stderr, "error attempting to bind lwp to processor [%d]\n",(int) procId);
		lwpBoundP = 1;

	    } else {

		if (obind == PBIND_NONE) {				// Couldn't bind to lwp
		    //
		    fprintf(stderr, "couldn't bind current lwp to processor [%d]\n", (int) procId);

		} else {

		    fprintf(stderr,"lwp bound to processor [%d]\n",procId);
		    lwpBoundP = 1;
		    *processorId = procId;
		}
	    }
	}
    }
}


//
Bool   pth_maybe_acquire_lock   (Lock lock)   {
    // ===========
    //
    // Return FALSE if cannot set lock;
    // otherwise set lock and return TRUE.
    // Created: 5-14-96 	
    // Invariant: If more than one processes calls pth_maybe_acquire_lock at the same time, 
    //		  then only one of the processes will have TRUE returned.

    #if defined(MP_PROFILE)
        long cpuTime;
    #endif

    #if defined(MP_LOCK_DEBUG)
        printf("pth_maybe_acquire_lock: lock value is %d\n",lock->value);
    #endif

    #if defined(MP_PROFILE)
        if (*doProfile) {
	    cpuTime = (long) clock();
	    printf("trylock_calls = %d\n",++trylock_calls);
	}
    #endif

    // We test to see if the lock is set here so that we can reduce the number
    // of calls to mutex_trylock when we are waiting for the lock to be 
    // released. Apparently repeated calls to mutex_trylock floods the bus.
    // I don't know why. I found this out from the Threads Primer book.

    if (lock->value == SET) {
	#if defined(MP_PROFILE)
	    if (*doProfile)   fprintf(stderr,"MP_Trylock:cpu time %ld\n",(long) clock() - cpuTime);
	    return FALSE;
	#else
	    return FALSE;
	#endif

    } else {

	#if defined(MP_LOCK_DEBUG)
	    printf("pth_maybe_acquire_lock: calling mutex_trylock\n");
	#endif

	#if defined(MP_PROFILE)
	    if (*doProfile)   printf("mutex_trylock_calls = %d\n",++mutex_trylock_calls);
	#endif

	if (mutex_trylock(&lock->mutex) == EBUSY) {
	    //
	    #if defined(MP_PROFILE)
		if (*doProfile)   fprintf(stderr,"MP_Trylock:cpu time %ld\n",(long) clock() - cpuTime);
	    #else
		    return(FALSE);
	    #endif

	} else {

	    if (lock->value == SET) {
		//
		mutex_unlock(&lock->mutex);
		//
		#if defined(MP_PROFILE)
		    if (*doProfile)   fprintf(stderr,"MP_Trylock:cpu time %ld\n",(long) clock() - cpuTime);
		#endif
		return(FALSE);
	    }

	    lock->value = SET;
	    mutex_unlock(&lock->mutex);

	    #if defined(MP_PROFILE)
		if (*doProfile)   fprintf(stderr,"MP_Trylock:cpu time %ld\n",(long) clock() - cpuTime);
	    #endif

	    return TRUE;
	}
    }
}						// fun pth_maybe_acquire_lock


//
void   pth_release_lock   (Lock lock)   {
    // ===============
    //
    // Assign lock->value the value of 0.
    // Created: 5-14-96 	   

    lock->value = UNSET;
} 
//
void   pth_acquire_lock    (Lock lock)   {
    // ===============
    //
    // Busy wait until able set the lock.
    // Created: 5-14-96 	   
    //
    while (pth_maybe_acquire_lock(lock) == FALSE);
} 


//
Lock   pth_make_lock   ()   {
    // ============
    //
    Lock lock;

    pth_acquire_lock(arena_lock_local);
       lock = allocate_lock();
    pth_release_lock(arena_lock_local);

    return lock;
}

//
void   pth_free_lock   (Lock lock)   {
    // ============
    //
    // Destroy mutex of lock and free memory occupied by lock.
    // Return non-negative int if OK, -1 on error.
    // Created: 5-13-96 	   

    pth_acquire_lock(arena_lock_local);
	//
	free_lock( lock );
	//
    pth_release_lock(arena_lock_local);
} 


//
static Barrier*   allocate_barrier   ()   {
    //            ================
    //
    // Get a chunk of memory from the arena for a barrier and initialize it.
    // Return a pointer to the barrier.
    // Created: 5-15-96 	   

    Barrier*  barrierp =  (Barrier*) arena_local;

    arena_local += MP_BARRIER_SZ;

    barrierp->n_waiting = 0; 
    barrierp->phase     = 0; 

    if (mutex_init(&barrierp->lock,   USYNC_THREAD, NULL) == -1)      die("pth_wait_at_barrier: could not init barrier mutex lock");
    if (cond_init(&barrierp->wait_cv, USYNC_THREAD, NULL) == -1)      die("pth_wait_at_barrier: Could not init conditional var of barrier");

    return barrierp;
}

//
Barrier*   pth_make_barrier   ()   {
    //     ===============
    //
    // Allocate a barrier from the synch chunk arena.
    // Allocation is mutually exclusive.
    // 
    // Return a pointer to the barrier.
    // 
    // Note that the barrier is not initialized.
    // Created: 5-15-96 	   

    Barrier* barrierp;

    pth_acquire_lock(arena_lock_local);
	barrierp = allocate_barrier ();
    pth_release_lock(arena_lock_local);

    return barrierp;
}
//
static void   free_barrier   (Barrier* barrierp)   {
    //        ============
    //
    // Destroy mutex and conditional variables of the barrier.
    // Regain memory if barrier was last chunk allocated in arena;
    // otherwise zero out the memory occupied by the barrier.
    // Created: 5-15-96 	   

    mutex_destroy(&barrierp->lock);
    cond_destroy(&barrierp->wait_cv);

    free_arena_ram(barrierp, MP_BARRIER_SZ);
}


//
void   pth_free_barrier  (Barrier* barrierp)   {
    // ===============
    //
    pth_acquire_lock(arena_lock_local);
       free_barrier(barrierp);
    pth_release_lock(arena_lock_local);
}

//
void   pth_wait_at_barrier   (Barrier* barrierp,  unsigned n_clients)   {
    // ==========
    //
    // Wait until the required number of threads enter the barrier.
    // Invariant: barrierp->n_waiting <= n_clients	   
    // Created: 5-15-96 


    mutex_lock(&barrierp->lock);

    int my_phase =  barrierp->phase;

    ++ barrierp->n_waiting;

    if (barrierp->n_waiting == n_clients)  {
	barrierp->n_waiting = 0;
	barrierp->phase = 1 - my_phase;
	cond_broadcast( &barrierp->wait_cv );
    }

    // Wait for the end of this synchronization phase
    //
    while (barrierp->phase == my_phase)  {
	//
	cond_wait(&barrierp->wait_cv, &barrierp->lock);
    }

    mutex_unlock(&barrierp->lock);
}
//
void   pth_reset_barrier   (Barrier* barrierp)   {
    // ================
    //
    // Set the various values of the barrier to zero.
    // Created: 5-15-96 	   

    barrierp->n_waiting = 0; 
    barrierp->phase = 0; 
}


//
static void*   allocate_arena_ram   (int size)   {
    //         ==================
    //
    void*  obj = arena_local;

    arena_local += size;

    return obj;
}
//
static void   free_arena_ram   (void* p,  int size)   {
    //        ==============
    //
    if (arena_local == (caddr_t) p + size)      arena_local -= size;
    else 				 	memset( p, 0, size );
}

//
static void*   resume_pthread   (void* vtask)   {
    //         ==============
    //
    // Resumes a proc to either perform garbage collection or to 
    // run ml with the given ml state.

    Task* task = (Task*) vtask;

    pth_acquire_lock(mp_pthread_lock_local);

    if (task->pthread->status == KERNEL_THREAD_IS_SUSPENDED) {
	//
	// Proc only resumed to do a clean.
	//
	#ifdef MULTICORE_SUPPORT_DEBUG
	      debug_say("resuming %d to perform a gc\n",task->pthread->pid);
	#endif      

	task->pthread->status == MP_PROC_GC;

	pth_release_lock( mp_pthread_lock_local );

	// The clean will be performed when we call pth_release_pthread

	pth_release_pthread( task );

    } else {

	#ifdef MULTICORE_SUPPORT_DEBUG
	      debug_say("[release_pthread: resuming proc %d]\n",task->pthread->pid);
	#endif

	pth_release_lock(mp_pthread_lock_local);

	run_mythryl_task_and_runtime_eventloop( task );								// run_mythryl_task_and_runtime_eventloop		def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c

	die ("return after run_mythryl_task_and_runtime_eventloop(task) in mp_release_pthread\n");
    }
}

//
Pthread*   resume_pthreads   (int n_procs)   {
    //     ===============
    //
    // Remove n_procs states from the list of states
    // and spawn threads to execute them. 
    //
    // Return a pointer to the last state resumed.
    //
    // Note: We assume that calls to this function are mutually exclusive.

    Task* statep =  (Task*) NULL

    for (int i = 0;  i < MAX_PTHREADS  &&  n_procs > 0;   ++i) {
	//
        if ((statep = tasks_local[i]) != (Task*) NULL) {		// Get a state.

	    // Spawn a thread to execute the state:

	    #ifdef MULTICORE_SUPPORT_DEBUG
		debug_say("Resuming proc %d\n",statep->pthread->pid);
	    #endif	

	    if (thr_create(NULL,0,resume_pthread,(void *)statep,NULL,NULL) != 0) {
		//
	        die("Could create a thread to resume processors");		// XXX BUGGO FIXME Is this error message right?
	    }

	    tasks_local[i] = NULL;

	    n_procs--;
	}
    }

    if (statep == (Task*) NULL)   return (Pthread*) NULL;

    return statep->pthread;
}							// fun resume_pthreads
//
static void   suspend_pthread   (Task* task) {
    //        ===============
    //
    // Suspend the calling proc, add its state, task, to the suspended
    // proc state list, and kill the pthread's kernel thread.

    int i = 0;

    pth_acquire_lock( mp_pthread_lock_local );

    // Check if pthread has actually been suspended:
    //
    if (task->pthread->status != KERNEL_THREAD_IS_SUSPENDED) {
	//
        #ifdef MULTICORE_SUPPORT_DEBUG
	    debug_say("proc state is not PROC_SUSPENDED; not suspended");
        #endif      

	pth_release_lock(mp_pthread_lock_local);

	return;
    }


    while (i < MAX_PTHREADS) { 
	//
        if (tasks_local[i] != NULL) {
	    i++;
        } else {
	    tasks_local[i] = task; 
	    i = MAX_PTHREADS;
	}
    }

    pth_release_lock(mp_pthread_lock_local);

    thr_exit(NULL);				// Exit the thread.
}						// fun suspend_pthread.
//
void   pth_release_pthread   (Task* task)   {
    // ==================
    //
    clean_heap( task, 1 );

    pth_acquire_lock(mp_pthread_lock_local);
       task->pthread->status = KERNEL_THREAD_IS_SUSPENDED;
    pth_release_lock(mp_pthread_lock_local);

    // Suspend the proc:
    //
    #ifdef MULTICORE_SUPPORT_DEBUG
        debug_say("suspending proc %d\n",task->pthread->pid);
    #endif
    suspend_pthread( task );
}
//
static void*   pthread_main   (void* vtask)   {
    //         ============
    //
    // Invoke run_mythryl_task_and_runtime_eventloop on task; die if it returns.

    Task* task = (Task*) vtask;

    // Spin until we get our id
    // (from return of call to thr_create):
    //
    while  (task->pthread->pid == NULL) {
	//
	#ifdef MULTICORE_SUPPORT_DEBUG
	    debug_say("[waiting for self]\n");
	#endif
	continue;
    }
    #ifdef MULTICORE_SUPPORT_DEBUG
        debug_say ("[new proc main: releasing lock]\n");
    #endif

    bind_to_kernel_thread( processorId );

    pth_release_lock(mp_pthread_lock_local);		// Implicitly handed to us by the parent.

    run_mythryl_task_and_runtime_eventloop( task );			// run_mythryl_task_and_runtime_eventloop		def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c

    // run_mythryl_task_and_runtime_eventloop() should never return:
    //
    die("proc returned after run_lib7() in pthread_main().\n");
}


//
Val   pth_acquire_pthread   (Task* task, Val arg)   {
    //==================
    //
    Task* p;
    Pthread* pthread;
    Val v = GET_TUPLE_SLOT_AS_VAL(arg, 0);
    Val f = GET_TUPLE_SLOT_AS_VAL(arg, 1);
    int i;

    #ifdef MULTICORE_SUPPORT_DEBUG
        debug_say("[acquiring proc]\n");
    #endif

    pth_acquire_lock(mp_pthread_lock_local);

    // Search for a suspended proc to reuse:
    //
    for (i = 0;
	 (i < pthread_count_global) && (pthread_table_global[i]->status != KERNEL_THREAD_IS_SUSPENDED);
	 i++)
      continue;

    #ifdef MULTICORE_SUPPORT_DEBUG
        debug_say("[checking for suspended processor]\n");
    #endif

    if (i == pthread_count_global) {
	//
        if (DEREF( ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL )  ==  TAGGED_INT_FROM_C_INT( MAX_PTHREADS )) {
	    //
	    pth_release_lock(mp_pthread_lock_local);
	    say_error("[processors maxed]\n");
	    return HEAP_FALSE;
	}

        #ifdef MULTICORE_SUPPORT_DEBUG
	    debug_say("[checking for NO_PROC]\n");
        #endif

	// Search for a slot in which to put a new proc:
        //
	for (i = 0;
	     (i < pthread_count_global) && (pthread_table_global[i]->status != NO_KERNEL_THREAD_ALLOCATED);
	     i++)
	  continue;

        if (i == pthread_count_global) {
	    //
	    pth_release_lock(mp_pthread_lock_local);
	    say_error( "[no processor to allocate]\n" );
	    return HEAP_FALSE;
	}

        pthread = pthread_table_global[i];      // Use pthread at index i.

    } else {

	// Using a suspended processor.

	#ifdef MULTICORE_SUPPORT_DEBUG
	    debug_say("[using a suspended processor]\n");
	#endif     

	pthread =  resume_pthreads(1);
    }

    p = pthread->task;

    p->exception_fate	=  PTR_CAST( Val,  handle_v + 1 );
    p->argument		=  HEAP_VOID;
    p->fate		=  PTR_CAST( Val,  return_to_c_level_c );
    p->closure		=  f;
    p->thread		=  v;
    //
    p->program_counter	= 
    p->link_register	=  GET_CODE_ADDRESS_FROM_CLOSURE( f );

    if (pthread->status == NO_KERNEL_THREAD_ALLOCATED) {
        //
	Pid  procId;

	// Assume we get one:
        //
	ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL, INT_LIB7inc(DEREF( ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL ), 1));

	if (thr_create( NULL, 0, pthread_main, (void*)p, THR_NEW_LWP, &((thread_t) procId)) == 0) {
	    //
	    #ifdef MULTICORE_SUPPORT_DEBUG
	        debug_say ("[got a processor: %d,]\n",procId);
	    #endif

	    pthread->status = KERNEL_THREAD_IS_RUNNING;
	    pthread->pid = procId;

	    // make_pthread will release mp_pthread_lock_local.

	    return HEAP_TRUE;

	} else {

	    ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL, INT_LIB7dec(DEREF( ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL ), 1) );
	    pth_release_lock(mp_pthread_lock_local);
	    return HEAP_FALSE;
	}

    } else {

	// The thread executing the processor
	// has already been invoked:

	pthread->status = KERNEL_THREAD_IS_RUNNING;

	#ifdef MULTICORE_SUPPORT_DEBUG
	    debug_say ("[reusing a processor %d]\n", pthread->pid);
	#endif

	pth_release_lock( mp_pthread_lock_local );

	return  HEAP_TRUE;
    }
}							// fun pth_acquire_pthread

//
void   pth_shut_down      (void)   {	munmap(arena_local,sysconf(_SC_PAGESIZE));	}
int    pth_max_pthreads   (void)   {	return MAX_PTHREADS;				}
Pid    pth_pthread_id     (void)   {	return (thr_self());				}    // Called only from:    src/c/main/runtime-state.c
    //

//
int   pth_active_pthread_count   (void)   {
    //=======================
    //
    pth_acquire_lock(mp_pthread_lock_local);
        //
        int ap = TAGGED_INT_TO_C_INT(DEREF( ACTIVE_PTHREADS_COUNT_REFCELL_GLOBAL ));
	//
    pth_release_lock(mp_pthread_lock_local);
    //
    return ap;
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

