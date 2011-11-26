// pthread-on-solaris.c
//
// This is an ancient (circa 1994?) implementation of pthread on top
// of the solaris operating system of the era.  These days Solaris
// must have a standard posix-threads implementation, so this
// file should be obsoleted by   src/c/pthread/pthread-on-posix-threads.c
// and should probably be deleted by and by. For the moment I'm keeping
// it around for historical interest and comparison during debugging.
//
// (In any event, the API defined by the pthread section of
//
//     src/c/h/runtime-base.h
//
// contains many changes not reflected in this file.)
//
// Multicore (well, multiprocessor) support for Sparc32 multiprocessor machines running Solaris 2.5
//
// Solaris implementation of externals defined in $(INCLUDE)/runtime-base.h


#include "../mythryl-config.h"

#include <stdio.h>
#include <string.h>
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
#include "runtime-base.h"
#include "runtime-globals.h"


#define INT_LIB7inc(n,i)  ((Val)TAGGED_INT_FROM_C_INT(TAGGED_INT_TO_C_INT(n) + (i)))
#define INT_LIB7dec(n,i)  (INT_LIB7inc(n,(-i)))

int   pth__done_pthread_create = FALSE;

 static Mutex 	 allocate_mutex	();
 static Barrier* allocate_barrier	();
 static void*    allocate_arena_ram	(int size);
 static void     free_arena_ram		(void*, int);
 static void*    pthread_main		(void* task);
 static void*    resume_pthread		(void* vtask);
 static void     suspend_pthread	(Task* task);
 static Task**   initialize_task_vector	();
 static void     bind_to_kernel_thread	(processorid_t*);

static caddr_t          arena__local;				// Arena for shared sync chunks.
static Mutex        arena_mutex__local;				// Must be held to alloc/free a mutex.
static Mutex   mp_pthread_mutex__local;				// Must be used to acquire/release procs.
static Task**           tasks__local; /*[MAX_PTHREADS]*/		// List of states of suspended pthreads.

#if defined(MP_PROFILE)
    static int *doProfile;
#endif

#define LEAST_PROCESSOR_ID       0
#define GREATEST_PROCESSOR_ID    3

#define NextProcessorId(id)  (((id) == GREATEST_PROCESSOR_ID) ? LEAST_PROCESSOR_ID : (id) + 1)

static processorid_t* processorId;		// processor id of the next processor a lwp will be bound to globals.

Mutex	 pth__heapcleaner_mutex;
Mutex	 pth__heapcleaner_gen_mutex;
Mutex	 pth__timer_mutex;
Barrier* pth__heapcleaner_barrier;

#if defined(MP_PROFILE)
    int mutex_trylock_calls;
    int trylock_calls;
#endif



void   pth__start_up   ()   {
    // =============
    //
    // Called (only) from   src/c/main/runtime-main.c

    int fd;

    if ((fd = open("/dev/zero",O_RDWR)) == -1)   die("pth__start_up:Couldn't open /dev/zero");

    arena__local = mmap((caddr_t) 0, sysconf(_SC_PAGESIZE),PROT_READ | PROT_WRITE ,MAP_PRIVATE,fd,0);

    arena_mutex__local			= allocate_mutex();
    mp_pthread_mutex__local		= allocate_mutex();
    pth__heapcleaner_mutex	= allocate_mutex();
    pth__heapcleaner_gen_mutex	= allocate_mutex();
    pth__timer_mutex		= allocate_mutex();
    //
    pth__heapcleaner_barrier	= allocate_barrier(); 
    //
    tasks__local			= initialize_task_vector();
    //
    ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL, TAGGED_INT_FROM_C_INT(1) );

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
static Mutex    allocate_mutex   ()   {
    //	       =============
    //	
    // Allocate a portion of the arena of synch chunks for a spin lock.
    // Return a pointer to the allocated region.
    // Created: 5-14-96 	   

    Mutex  mutex = (Mutex) allocate_arena_ram(MP_MUTEX_SZ);

    mutex->value = UNSET;

    if (mutex_init(&mutex->mutex, USYNC_THREAD, NULL) == -1)      die("allocate_mutex: unable to initialize mutex");

    return mutex;
}


//
static void   free_mutex   (Mutex mutex)   {
    //        =========
    //
    // Destroy the mutex. In addition, if the mutex was the last chunk 
    // allocated in the arena then recapture the space occupied by the 
    // mutex. Otherwise, zero out the space occupied by the mutex.
    // Created 5-14-96

    #if defined(MP_MUTEX_DEBUG)
        printf("arena = %ld\t mutex = %ld\n",(int) arena__local, mutex);
    #endif
 
    mutex_destroy(&mutex->mutex);

    free_arena_ram(mutex,MP_MUTEX_SZ);
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
Bool   pth__mutex_maybe_lock   (Mutex mutex)   {
    // ===========
    //
    // Return FALSE if cannot set mutex;
    // otherwise set mutex and return TRUE.
    // Created: 5-14-96 	
    // Invariant: If more than one processes calls pth__mutex_maybe_lock at the same time, 
    //		  then only one of the processes will have TRUE returned.

    #if defined(MP_PROFILE)
        long cpuTime;
    #endif

    #if defined(MP_MUTEX_DEBUG)
        printf("pth__mutex_maybe_lock: mutex value is %d\n",mutex->value);
    #endif

    #if defined(MP_PROFILE)
        if (*doProfile) {
	    cpuTime = (long) clock();
	    printf("trylock_calls = %d\n",++trylock_calls);
	}
    #endif

    // We test to see if the mutex is set here so that we can reduce the number
    // of calls to mutex_trylock when we are waiting for the mutex to be 
    // released. Apparently repeated calls to mutex_trylock floods the bus.
    // I don't know why. I found this out from the Threads Primer book.

    if (mutex->value == SET) {
	#if defined(MP_PROFILE)
	    if (*doProfile)   fprintf(stderr,"MP_Trylock:cpu time %ld\n",(long) clock() - cpuTime);
	    return FALSE;
	#else
	    return FALSE;
	#endif

    } else {

	#if defined(MP_MUTEX_DEBUG)
	    printf("pth__mutex_maybe_lock: calling mutex_trylock\n");
	#endif

	#if defined(MP_PROFILE)
	    if (*doProfile)   printf("mutex_trylock_calls = %d\n",++mutex_trylock_calls);
	#endif

	if (mutex_trylock(&mutex->mutex) == EBUSY) {
	    //
	    #if defined(MP_PROFILE)
		if (*doProfile)   fprintf(stderr,"MP_Trylock:cpu time %ld\n",(long) clock() - cpuTime);
	    #else
		    return(FALSE);
	    #endif

	} else {

	    if (mutex->value == SET) {
		//
		mutex_unlock(&mutex->mutex);
		//
		#if defined(MP_PROFILE)
		    if (*doProfile)   fprintf(stderr,"MP_Trylock:cpu time %ld\n",(long) clock() - cpuTime);
		#endif
		return(FALSE);
	    }

	    mutex->value = SET;
	    mutex_unlock(&mutex->mutex);

	    #if defined(MP_PROFILE)
		if (*doProfile)   fprintf(stderr,"MP_Trylock:cpu time %ld\n",(long) clock() - cpuTime);
	    #endif

	    return TRUE;
	}
    }
}						// fun pth__mutex_maybe_lock


//
void   pth__mutex_unlock   (Mutex mutex)   {
    // ===============
    //
    // Assign mutex->value the value of 0.
    // Created: 5-14-96 	   

    mutex->value = UNSET;
} 
//
void   pth__mutex_lock    (Mutex mutex)   {
    // ===============
    //
    // Busy wait until able set the mutex.
    // Created: 5-14-96 	   
    //
    while (pth__mutex_maybe_lock(mutex) == FALSE);
} 


//
Mutex   pth__make_mutex   ()   {
    // ============
    //
    Mutex mutex;

    pth__mutex_lock(arena_mutex__local);
       mutex = allocate_mutex();
    pth__mutex_unlock(arena_mutex__local);

    return mutex;
}

//
void   pth__free_mutex   (Mutex mutex)   {
    // ============
    //
    // Destroy mutex of mutex and free memory occupied by mutex.
    // Return non-negative int if OK, -1 on error.
    // Created: 5-13-96 	   

    pth__mutex_lock(arena_mutex__local);
	//
	free_mutex( mutex );
	//
    pth__mutex_unlock(arena_mutex__local);
} 


//
static Barrier*   allocate_barrier   ()   {
    //            ================
    //
    // Get a chunk of memory from the arena for a barrier and initialize it.
    // Return a pointer to the barrier.
    // Created: 5-15-96 	   

    Barrier*  barrierp =  (Barrier*) arena__local;

    arena__local += MP_BARRIER_SZ;

    barrierp->n_waiting = 0; 
    barrierp->phase     = 0; 

    if (mutex_init(&barrierp->mutex,   USYNC_THREAD, NULL) == -1)      die("pth__wait_at_barrier: could not init barrier mutex mutex");
    if (cond_init(&barrierp->wait_cv, USYNC_THREAD, NULL) == -1)      die("pth__wait_at_barrier: Could not init conditional var of barrier");

    return barrierp;
}

//
Barrier*   pth__make_barrier   ()   {
    //     =================
    //
    // Allocate a barrier from the synch chunk arena.
    // Allocation is mutually exclusive.
    // 
    // Return a pointer to the barrier.
    // 
    // Note that the barrier is not initialized.
    // Created: 5-15-96 	   

    Barrier* barrierp;

    pth__mutex_lock(arena_mutex__local);
	barrierp = allocate_barrier ();
    pth__mutex_unlock(arena_mutex__local);

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

    mutex_destroy(&barrierp->mutex);
    cond_destroy(&barrierp->wait_cv);

    free_arena_ram(barrierp, MP_BARRIER_SZ);
}


//
void   pth__free_barrier  (Barrier* barrierp)   {
    // =================
    //
    pth__mutex_lock(arena_mutex__local);
       free_barrier(barrierp);
    pth__mutex_unlock(arena_mutex__local);
}

//
void   pth__wait_at_barrier   (Barrier* barrierp,  unsigned n_clients)   {
    // ==========
    //
    // Wait until the required number of threads enter the barrier.
    // Invariant: barrierp->n_waiting <= n_clients	   
    // Created: 5-15-96 


    mutex_mutex(&barrierp->mutex);

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
	cond_wait(&barrierp->wait_cv, &barrierp->mutex);
    }

    mutex_unlock(&barrierp->mutex);
}
//
void   pth__clear_barrier   (Barrier* barrierp)   {
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
    void*  obj = arena__local;

    arena__local += size;

    return obj;
}
//
static void   free_arena_ram   (void* p,  int size)   {
    //        ==============
    //
    if (arena__local == (caddr_t) p + size)      arena__local -= size;
    else 				 	memset( p, 0, size );
}

//
static void*   resume_pthread   (void* vtask)   {
    //         ==============
    //
    // Resumes a proc to either perform garbage collection or to 
    // run ml with the given ml state.

    Task* task = (Task*) vtask;

    pth__mutex_lock(mp_pthread_mutex__local);

    if (task->pthread->mode == IS_BLOCKED) {
	//
	// Proc only resumed to do a clean.
	//

	PTHREAD_LOG_IF ("resuming %d to perform a gc\n",task->pthread->pid);

	task->pthread->mode == IS_HEAPCLEANING;

	pth__mutex_unlock( mp_pthread_mutex__local );

	// The clean will be performed when we call pth__pthread_exit

	pth__pthread_exit( task );

    } else {

	PTHREAD_LOG_IF ("[release_pthread: resuming proc %d]\n",task->pthread->pid);

	pth__mutex_unlock(mp_pthread_mutex__local);

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
        if ((statep = tasks__local[i]) != (Task*) NULL) {		// Get a state.

	    // Spawn a thread to execute the state:

	    PTHREAD_LOG_IF ("Resuming proc %d\n",statep->pthread->pid);

	    if (thr_create(NULL,0,resume_pthread,(void *)statep,NULL,NULL) != 0) {
		//
	        die("Could create a thread to resume processors");		// XXX BUGGO FIXME Is this error message right?
	    }

	    tasks__local[i] = NULL;

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

    pth__mutex_lock( mp_pthread_mutex__local );

    // Check if pthread has actually been suspended:
    //
    if (task->pthread->mode != IS_BLOCKED) {
	//
	PTHREAD_LOG_IF ("proc state is not PROC_SUSPENDED; not suspended");

	pth__mutex_unlock( &mp_pthread_mutex__local );

	return;
    }


    while (i < MAX_PTHREADS) { 
	//
        if (tasks__local[i] != NULL) {
	    i++;
        } else {
	    tasks__local[i] = task; 
	    i = MAX_PTHREADS;
	}
    }

    pth__mutex_unlock(mp_pthread_mutex__local);

    thr_exit(NULL);				// Exit the thread.
}						// fun suspend_pthread.
//
void   pth__pthread_exit   (Task* task)   {
    // ===================
    //
    call_heapcleaner( task, 1 );							// call_heapcleaner		def in   /src/c/heapcleaner/call-heapcleaner.c

    pth__mutex_lock(mp_pthread_mutex__local);
       task->pthread->mode = PTHREAD_IS_BLOCKED;
    pth__mutex_unlock(mp_pthread_mutex__local);

    // Suspend the proc:
    //
    PTHREAD_LOG_IF ("suspending proc %d\n", task->pthread->pid);

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
	PTHREAD_LOG_IF ("[waiting for self]\n");
	//
	continue;
    }

    PTHREAD_LOG_IF ("[new proc main: releasing mutex]\n");

    bind_to_kernel_thread( processorId );

    pth__mutex_unlock(mp_pthread_mutex__local);		// Implicitly handed to us by the parent.

    run_mythryl_task_and_runtime_eventloop( task );			// run_mythryl_task_and_runtime_eventloop		def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c

    // run_mythryl_task_and_runtime_eventloop() should never return:
    //
    die("proc returned after run_lib7() in pthread_main().\n");
}


//
Val   pth__pthread_create   (Task* task, Val arg)   {
    //====================
    //
    pth__done_pthread_create = TRUE;

    Task* p;
    Pthread* pthread;
    Val v = GET_TUPLE_SLOT_AS_VAL(arg, 0);	// current_thread nowadays, mv_varReg originally.
    Val f = GET_TUPLE_SLOT_AS_VAL(arg, 1);	// closure
    int i;

    PTHREAD_LOG_IF ("[acquiring proc]\n");

    pth__mutex_lock(mp_pthread_mutex__local);

    // Search for a suspended proc to reuse:
    //
    for (  i = 0;
	   (i < MAX_PTHREADS)  &&  (pthread_table__global[i]->status != PTHREAD_IS_SUSPENDED);
	   i++
	);


    PTHREAD_LOG_IF ("[checking for suspended processor]\n");

    if (i == MAX_PTHREADS) {
	//
        if (DEREF( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL )  ==  TAGGED_INT_FROM_C_INT( MAX_PTHREADS )) {
	    //
	    pth__mutex_unlock(mp_pthread_mutex__local);
	    say_error("[processors maxed]\n");
	    return HEAP_FALSE;
	}

	PTHREAD_LOG_IF ("[checking for NO_PROC]\n");

	// Search for a slot in which to put a new proc:
        //
	for (  i = 0;
	      (i < MAX_PTHREADS)  &&  (pthread_table__global[i]->status != IS_VOID);
	       i++
            );

        if (i == MAX_PTHREADS) {
	    //
	    pth__mutex_unlock(mp_pthread_mutex__local);
	    say_error( "[no processor to allocate]\n" );
	    //
	    return HEAP_FALSE;
	}

        pthread = pthread_table__global[i];      // Use pthread at index i.

    } else {

	// Using a suspended processor.

	PTHREAD_LOG_IF ("[using a suspended processor]\n");

	pthread =  resume_pthreads(1);
    }

    p = pthread->task;

    p->exception_fate	=  PTR_CAST( Val,  handle_v + 1 );
    p->argument		=  HEAP_VOID;
    p->fate		=  PTR_CAST( Val,  return_to_c_level_c );
    p->current_closure	=  f;
    p->current_thread	=  v;
    //
    p->program_counter	= 
    p->link_register	=  GET_CODE_ADDRESS_FROM_CLOSURE( f );

    if (pthread->mode == IS_VOID) {
        //
	Pid  procId;

	// Assume we get one:
        //
	ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL, INT_LIB7inc(DEREF( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL ), 1));

	if (thr_create( NULL, 0, pthread_main, (void*)p, THR_NEW_LWP, &((thread_t) procId)) == 0) {
	    //
	    PTHREAD_LOG_IF ("[got a processor: %d,]\n",procId);

	    pthread->mode = IS_RUNNING;
	    pthread->pid = procId;

	    // make_pthread will release mp_pthread_mutex__local.

	    return HEAP_TRUE;

	} else {

	    ASSIGN( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL, INT_LIB7dec(DEREF( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL ), 1) );
	    pth__mutex_unlock(mp_pthread_mutex__local);
	    return HEAP_FALSE;
	}

    } else {

	// The thread executing the processor
	// has already been invoked:

	pthread->mode = IS_RUNNING;

	PTHREAD_LOG_IF ("[reusing a processor %d]\n", pthread->pid);

	pth__mutex_unlock( mp_pthread_mutex__local );

	return  HEAP_TRUE;
    }
}							// fun pth__pthread_create

//
void   pth__shut_down		(void)   {	munmap(arena__local,sysconf(_SC_PAGESIZE));	}
int    pth__max_pthreads	(void)   {	return MAX_PTHREADS;				}
Pid    pth__get_pthread_id	(void)   {	return (thr_self());				}
    //

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
	if (pthread_table__global[i]->pid == pid)   return &pthread_table__global[ i ];	// pthread_table__global	def in   src/c/main/runtime-state.c
    }											// pthread_table__global exported via     src/c/h/runtime-base.h
    die "pth__get_pthread:  pid %d not found in pthread_table__global?!", pid;
#endif
}

//
int   pth__get_active_pthread_count   (void)   {
    //=======================
    //
    pth__mutex_lock(mp_pthread_mutex__local);
        //
        int ap = TAGGED_INT_TO_C_INT(DEREF( ACTIVE_PTHREADS_COUNT_REFCELL__GLOBAL ));
	//
    pth__mutex_unlock(mp_pthread_mutex__local);
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

