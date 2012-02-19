// libmythryl-pthread.c
//
// For background see
//
//     src/A.PTHREAD-SUPPORT.OVERVIEW
//
// and the docs at the bottom of
// 
//     src/c/pthread/pthread-on-posix-threads.c
//
//
//
// This file defines the "pthread" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my spawn_pthread:   Fate -> Bool
//         =
//         mythryl_callable_c_library_interface::find_c_function  { lib_name => "pthread", fun_name => "spawn_pthread" };
// 
// or such.

// Here we export to
//
//     src/lib/std/src/pthread.api
//     src/lib/std/src/pthread.pkg
//
// some of the functionality defined in the pthread section of
//
//     src/c/h/runtime-base.h
//
// and implemented in the platform-specific file
//
//     src/c/pthread/pthread-on-posix-threads.c



/*
### 			"Programs are forests of fatally poison ivy through which paths have been beaten; 
### 			 we peek off the path only to see our forebears' skeletons leering back at us."
###
###									-- Hue White  
*/ 



#include "../../mythryl-config.h"

#include <stdio.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-globals.h"

#include "make-strings-and-vectors-etc.h"
#include "mythryl-callable-c-libraries.h"

#include "../raise-error.h"

struct mutex_struct {
    //
    int					padding0[ CACHE_LINE_BYTESIZE / sizeof(int) ];		// See comment below.
    int      state;										// Track state: Uninitialized/initialized/cleared/free()d -- see below #defines.
    Mutex    mutex;
    int					padding1[ CACHE_LINE_BYTESIZE / sizeof(int) ];
};

struct condvar_struct {
    //
    int					padding0[ CACHE_LINE_BYTESIZE / sizeof(int) ];
    int      state;
    Condvar  condvar;
    int					padding1[ CACHE_LINE_BYTESIZE / sizeof(int) ];
};

struct barrier_struct {
    //
    int					padding0[ CACHE_LINE_BYTESIZE / sizeof(int) ];
    int      state;
    Barrier  barrier;
    int					padding1[ CACHE_LINE_BYTESIZE / sizeof(int) ];
};

    ////////////////////////////////////////////////////////////////
    // Why the padding[] arrays?
    //
    // We do not expect to have vast number of mutex, barrier
    // or condition variable, but obviously we do expect them
    // to be points of contention between cores.  In general
    // each core likes to lock down the relevant cache line
    // before doing any synchronization operations, so it pays
    // to make sure that each mutex, barrier and condition variable
    // is in its own cache line -- if we had multiple mutexes in the
    // same cacheline then separate cores operating on separate
    // mutexes, which should logically have no contention, might
    // wind up fighting for control of the shared cacheline.
    ////////////////////////////////////////////////////////////////

// Values for barrier_struct.state:
//
#define UNINITIALIZED_BARRIER	0xDEADBEEF
#define   INITIALIZED_BARRIER	0xBEEFFEED
#define       CLEARED_BARRIER	0xFEEDBEEF				// Same as UNINITIALIZED_BARRIER so far as posix-threads API is concerned, but distinguishing lets us issue more accurate diagnostics.
#define         FREED_BARRIER	0xDEADDEAD

// Values for condvar_struct.state:
//
#define UNINITIALIZED_CONDVAR	0xDEADBEEF
#define   INITIALIZED_CONDVAR	0xBEEFFEED
#define       CLEARED_CONDVAR	0xFEEDBEEF				// Same as UNINITIALIZED_CONDVAR so far as posix-threads API is concerned, but distinguishing lets us issue more accurate diagnostics.
#define         FREED_CONDVAR	0xDEADDEAD

// Values for mutex_struct.state:
//
#define UNINITIALIZED_MUTEX	0xDEADBEEF
#define   INITIALIZED_MUTEX	0xBEEFFEED
#define       CLEARED_MUTEX	0xFEEDBEEF				// Same as UNINITIALIZED_MUTEX   so far as posix-threads API is concerned, but distinguishing lets us issue more accurate diagnostics.
#define         FREED_MUTEX	0xDEADDEAD


// Probably should be using this in this file:    XXX SUCKO FIXME
// 
				///////////////////////////////////////////////////////
				// posix_memalign
				// 
				// SYNOPSIS 
				//       #include <stdlib.h> 
				// 
				//       int posix_memalign(void **memptr, size_t alignment, size_t size); 
				// 
				//       #include <malloc.h> 
				// 
				//       void *valloc(size_t size); 
				//       void *memalign(size_t boundary, size_t size); 
				// 
				//   Feature Test Macro Requirements for glibc (see feature_test_macros(7)): 
				// 
				//       posix_memalign(): _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600 
				// 
				//       valloc(): 
				//           Since glibc 2.12: 
				//               _BSD_SOURCE || 
				//                   (_XOPEN_SOURCE >= 500 || 
				//                       _XOPEN_SOURCE && _XOPEN_SOURCE_EXTENDED) && 
				//                   !(_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) 
				//           Before glibc 2.12: 
				//               _BSD_SOURCE || _XOPEN_SOURCE >= 500 || _XOPEN_SOURCE && _XOPEN_SOURCE_EXTENDED 
				// 
				// DESCRIPTION 
				//       The  function posix_memalign() allocates size bytes and places the address of the allocated memory in *memptr.
				//       The address of the allocated memory will be a multiple of alignment, which must be a power of two and a multiple 
				//       of sizeof(void *).  If size is 0, then posix_memalign() returns either NULL, or a unique pointer value that can later be successfully passed to free(). 
				///////////////////////////////////////////////////////



static Val   get_pthread_id         (Task* task,  Val arg)   {
    //       ==============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("get_pthread_id");

    #if NEED_PTHREAD_SUPPORT
	//
        return TAGGED_INT_FROM_C_INT( pth__get_pthread_id());			// thread_id	def in    src/c/pthread/pthread-on-posix-threads.c
        //									// thread_id	def in    src/c/pthread/pthread-on-sgi.c
    #else									// thread_id	def in    src/c/pthread/pthread-on-solaris.c
	die ("get_pthread_id: no mp support\n");
        return TAGGED_INT_FROM_C_INT( 0 );					// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   spawn_pthread   (Task* task,  Val closure)   {			// Apparently never called.
    //       =============
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("spawn_pthread");

  #if NEED_PTHREAD_SUPPORT
	//
//      Val current_thread =  GET_TUPLE_SLOT_AS_VAL( arg, 0 );			// This is stored into   pthread->task->current_thread.   NB: "task->current_thread" was "task->ml_varReg" back when this was written -- CML came later.
//      Val closure        =  GET_TUPLE_SLOT_AS_VAL( arg, 1 );			// This is stored into   pthread->task->current_closure

	int pthread_table_slot;							// An index into the   pthread_table__global[]   defined in   src/c/main/runtime-state.c
	char* err = pth__pthread_create( &pthread_table_slot, task->current_thread, closure );

	if (err)   return RAISE_ERROR__MAY_HEAPCLEAN(task, err, NULL);
	else	   return TAGGED_INT_FROM_C_INT( pthread_table_slot );
										// pth__pthread_create	def in    src/c/pthread/pthread-on-posix-threads.c
        //									// pth__pthread_create	def in    src/c/pthread/pthread-on-sgi.c
    #else									// pth__pthread_create	def in    src/c/pthread/pthread-on-solaris.c
	die ("spawn_pthread: no mp support\n");
        return HEAP_TRUE;							// Cannot execute; only present to quiet gcc.
    #endif
}



static Val   pthread_exit_fn   (Task* task,  Val arg)   {			// Name issues: The name 'pthread_exit' is used by <pthread.h>, and of course 'exit' by <stdlib.h>.
    //       ===============
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("pthread_exit_fn");

    #if NEED_PTHREAD_SUPPORT
	pth__pthread_exit(task);  	// Should not return.
	log_if("pthread_exit_fn: call unexpectedly returned\n");
	die(   "pthread_exit_fn: call unexpectedly returned\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #else
	die ("pthread_exit_fn: no mp support\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}


static Val   join_pthread   (Task* task,  Val pthread_to_join)   {		// Name issue: 'pthread_join' is used by <pthread.h>
    //       ============							// 'pthread_to_join' is a pthread_table__global[] index returned from a call to   spawn_pthread()   (above).
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("join_pthread");

    #if NEED_PTHREAD_SUPPORT
    {   char* err = pth__pthread_join( task, pthread_to_join );			// Used to pass TAGGED_INT_TO_C_INT( pthread_to_join ) );
	    //
	    if (err) {  log_if("join_pthread failed!");		return RAISE_ERROR__MAY_HEAPCLEAN(task, err,NULL );	}
	    else     {						return HEAP_VOID;			}
	}
    #else
	die ("join_pthread: no mp support\n");
      return HEAP_VOID;								// Cannot execute; only present to quiet gcc.
    #endif
}



static Val mutex_make   (Task* task,  Val arg)   {
    //     ==========
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("mutex_make");

    #if NEED_PTHREAD_SUPPORT
	//
	// We allocate the mutex_struct on the C
	// heap rather than the Mythryl heap because
	// having the garbage collector moving mutexes
	// around in memory seems like a really, really
	// bad idea:
	//
	struct mutex_struct*  mutex
	    =
	    (struct mutex_struct*)  MALLOC( sizeof(struct mutex_struct) );	if (!mutex) die("Unable to malloc mutex_struct"); 
		//
		//    "{malloc, calloc, realloc, free, posix_memalign} of glibc-2.2+ are thread safe"
		//
		//	-- http://linux.derkeiler.com/Newsgroups/comp.os.linux.development.apps/2005-07/0323.html


log_if("mutex_make malloc'd mutex %x", mutex);
	mutex->state = UNINITIALIZED_MUTEX;				// So we can catch attempts to wait on an uninitialized mutex at this level.

	// We return the address of the mutex_struct
	// to the Mythryl level encoded as a word value:
	//
        return  make_one_word_unt(task,  (Val_Sized_Unt)mutex  );

    #else
	die ("mutex_make: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   mutex_free   (Task* task,  Val arg)   {
    //       ==========
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("mutex_free");

    #if NEED_PTHREAD_SUPPORT
	// 'arg' should be something returned by barrier_make() above,
	// so it should be a Mythryl boxed word -- a two-word heap record
	// consisting of a tagword  MAKE_TAGWORD(1, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG)
	// followed by the C address of our   struct barrier_struct.
	// Per Mythryl convention, 'arg' will point to the second word,
	// so all we have to do is cast it appropriately:
	//
	struct mutex_struct*  mutex
	    =
	    *((struct mutex_struct**) arg);

	switch (mutex->state) {
	    //
	    case UNINITIALIZED_MUTEX:
	    case   INITIALIZED_MUTEX:
	    case       CLEARED_MUTEX:
		//
		mutex->state =  FREED_MUTEX;
log_if("mutex_free freeing %x", mutex);
		free( mutex );
		break;

	    case         FREED_MUTEX:
		log_if("Attempt to free already-freed mutex instance.");
		die(   "Attempt to free already-freed mutex instance.");

	    default:
		log_if("mutex_free: Attempt to free bogus value. (Already-freed mutex? Junk?)");
		die(   "mutex_free: Attempt to free bogus value. (Already-freed mutex? Junk?)");
	}
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
	//
    #else
	die ("mutex_free: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   mutex_init   (Task* task,  Val arg)   {
    //       ==========
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("mutex_init");

    #if NEED_PTHREAD_SUPPORT

	struct mutex_struct*  mutex
	    =
	    *((struct mutex_struct**) arg);

	switch (mutex->state) {
	    //
	    case UNINITIALIZED_MUTEX:
	    case       CLEARED_MUTEX:
		{   char* err = pth__mutex_init( task, arg, &mutex->mutex );
		    //
		    if (err)   {                                     return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );  }
		    else       { mutex->state = INITIALIZED_MUTEX;   return HEAP_VOID;                 }
		}
		break;

	    case   INITIALIZED_MUTEX:	log_if("Attempt to set already-set mutex.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to set already-set mutex.", NULL);
	    case         FREED_MUTEX:	log_if("Attempt to set freed mutex.");			return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to set freed mutex.", NULL);
	    default:			log_if("mutex_init: Attempt to set bogus value.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "mutex_init: Attempt to set bogus value. (Already-freed mutex? Junk?)", NULL);
	}

        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.

    #else
	die ("mutex_init: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   mutex_destroy   (Task* task,  Val arg)   {
    //       =============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("mutex_destroy");

    #if NEED_PTHREAD_SUPPORT

	struct mutex_struct*  mutex
	    =
	    *((struct mutex_struct**) arg);

	switch (mutex->state) {
	    //
	    case   INITIALIZED_MUTEX:
		{   char* err = pth__mutex_destroy( task, arg, &mutex->mutex );
		    //
		    if (err)   { log_if("mutex_destroy returned error");	return RAISE_ERROR__MAY_HEAPCLEAN(task, err, NULL);	}
		    else       { mutex->state = CLEARED_MUTEX;			return HEAP_VOID;			}
		}
		break;

	    case UNINITIALIZED_MUTEX:		log_if("Attempt to clear uninitialized mutex.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to clear uninitialized mutex.", NULL);
	    case       CLEARED_MUTEX:		log_if("Attempt to clear already-cleared mutex.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to clear already-cleared mutex.", NULL);
	    case         FREED_MUTEX:		log_if("Attempt to clear already-freed mutex.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to clear already-freed mutex.", NULL);
	    default:				log_if("mutex_destroy: Attempt to clear bogus value.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "mutex_destroy: Attempt to clear bogus value. (Already-freed mutex? Junk?)", NULL);
	}
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.

    #else
	die ("mutex_destroy: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   mutex_lock   (Task* task,  Val arg)   {
    //       ==========
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("mutex_lock");

if (running_script) log_if("mutex_lock: TOP...");
    #if NEED_PTHREAD_SUPPORT

	struct mutex_struct*  mutex
	    =
	    *((struct mutex_struct**) arg);

if (running_script) log_if("mutex_lock: AAA...");
	switch (mutex->state) {
	    //
	    case   INITIALIZED_MUTEX:
if (running_script) log_if("mutex_lock: BBB...");
		{    char* err =  pth__mutex_lock( task, arg, &mutex->mutex );
		    //
if (running_script) log_if("mutex_lock: CCC...");
		    if (err)   return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );
		    else       return HEAP_VOID;
		}
		break;

	    case UNINITIALIZED_MUTEX:	log_if("mutex_lock: Attempt to acquire mutex before setting it."); return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to acquire mutex before setting it.", NULL);
	    case       CLEARED_MUTEX:	log_if("mutex_lock: Attempt to acquire mutex after clearing it."); return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to acquire mutex after clearing it.", NULL);
	    case         FREED_MUTEX:	log_if("mutex_lock: Attempt to acquire mutex after freeing it.");  return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to acquire mutex after freeing it.", NULL);
	    default:			log_if("mutex_lock: mutex_lock: Attempt to acquire bogus value."); return RAISE_ERROR__MAY_HEAPCLEAN( task, "mutex_lock: Attempt to acquire bogus value. (Already-freed mutex? Junk?)", NULL);
	}
if (running_script) log_if("mutex_lock: YYY..."); 
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.

    #else
	die ("mutex_lock: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   mutex_unlock   (Task* task,  Val arg)   {
    //       ============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("mutex_unlock");

if (running_script) log_if("mutex_unlock: AAA...");
    #if NEED_PTHREAD_SUPPORT

	struct mutex_struct*  mutex
	    =
	    *((struct mutex_struct**) arg);

if (running_script) log_if("mutex_unlock: BBB...");
	switch (mutex->state) {
	    //
	    case   INITIALIZED_MUTEX:
if (running_script) log_if("mutex_unlock: CCC...");
		{   char* err =  pth__mutex_unlock( task, arg, &mutex->mutex );
if (running_script) log_if("mutex_unlock: DDD...");
		    //
		    if (err)   return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );
		    else       return HEAP_VOID;
		}
		break;

	    case UNINITIALIZED_MUTEX:	log_if("mutex_unlock: Attempt to release mutex before setting it.");	return RAISE_ERROR__MAY_HEAPCLEAN(task,"Attempt to release mutex before setting it.", NULL);
	    case       CLEARED_MUTEX:	log_if("mutex_unlock: Attempt to release mutex after clearing it.");	return RAISE_ERROR__MAY_HEAPCLEAN(task,"Attempt to release mutex after clearing it.", NULL);
	    case         FREED_MUTEX:	log_if("mutex_unlock: Attempt to release mutex after freeing it.");	return RAISE_ERROR__MAY_HEAPCLEAN(task,"Attempt to release mutex after freeing it.", NULL);
	    default:			log_if("mutex_unlock: Attempt to release bogus value.");		return RAISE_ERROR__MAY_HEAPCLEAN(task,"mutex_lock: Attempt to release bogus value. (Already-freed mutex? Junk?)", NULL);
	}
if (running_script) log_if("mutex_unlock: YYY...");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.

    #else
	die ("mutex_unlock: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   mutex_trylock   (Task* task,  Val arg)   {
    //       =============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("mutex_trylock");

    #if NEED_PTHREAD_SUPPORT

log_if("src/c/lib/pthread/libmythryl-pthread.c: mutex_trylock/AAA");
	struct mutex_struct*  mutex
	    =
	    *((struct mutex_struct**) arg);

log_if("src/c/lib/pthread/libmythryl-pthread.c: mutex_trylock/BBB");
	switch (mutex->state) {
	    //
	    case   INITIALIZED_MUTEX:
		{   Bool  result;
log_if("src/c/lib/pthread/libmythryl-pthread.c: mutex_trylock/CCC");
		    char* err = pth__mutex_trylock( task, arg, &mutex->mutex, &result );
log_if("src/c/lib/pthread/libmythryl-pthread.c: mutex_trylock/DDD");
		    //
		    if (err)		{	log_if("trylock returned error!");			return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
		    //
		    if (result)		{	log_if("trylock returning TRUE");			return HEAP_TRUE;			}	// Mutex was busy.
		    else		{	log_if("trylock returning FALSE");			return HEAP_FALSE;			}	// Successfully acquired mutex.
		};
		break;

	    case UNINITIALIZED_MUTEX:	log_if("mutex_trylock: Attempt to try mutex before setting it.");	return RAISE_ERROR__MAY_HEAPCLEAN(task,"Attempt to try mutex before setting it.", NULL);
	    case       CLEARED_MUTEX:	log_if("mutex_trylock: Attempt to try mutex after clearing it.");	return RAISE_ERROR__MAY_HEAPCLEAN(task,"Attempt to try mutex after clearing it.", NULL);
	    case         FREED_MUTEX:	log_if("mutex_trylock: Attempt to try mutex after freeing it.");	return RAISE_ERROR__MAY_HEAPCLEAN(task,"Attempt to try mutex after freeing it.", NULL) ;
	    default:			log_if("mutex_trylock: mutex_trylock: Attempt to try bogus value.");	return RAISE_ERROR__MAY_HEAPCLEAN(task,"mutex_trylock: Attempt to try bogus value. (Already-freed mutex? Junk?)", NULL );
	}
        return HEAP_VOID;							// Cannot execute.

    #else
	die ("mutex_trylock: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}



static Val   barrier_make   (Task* task,  Val arg)   {
    //       ============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("barrier_make");

    #if NEED_PTHREAD_SUPPORT
	//
	// We allocate the condvar_struct on the C
	// heap rather than the Mythryl heap because
	// having the garbage collector moving mutexes
	// around in memory seems like a really, really
	// bad idea:
	//
	struct barrier_struct*  barrier
	    =
	    (struct barrier_struct*)  MALLOC( sizeof(struct barrier_struct) );	if (!barrier) die("Unable to malloc barrier_struct"); 

log_if("barrier_make malloc()'d barrier %x", barrier);
	barrier->state = UNINITIALIZED_BARRIER;				// So we can catch attempts to wait on an uninitialized barrier at this level.

	// We return the address of the barrier_struct
	// to the Mythryl level encoded as a word value:
	//
        return  make_one_word_unt(task, (Val_Sized_Unt)barrier );

    #else
	die ("barrier_make: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   barrier_free   (Task* task,  Val arg)   {
    //       ============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("barrier_free");

   #if NEED_PTHREAD_SUPPORT
	// 'arg' should be something returned by barrier_make() above,
	// so it should be a Mythryl boxed word -- a two-word heap record
	// consisting of a tagword  MAKE_TAGWORD(1, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG)
	// followed by the C address of our   struct barrier_struct.
	// Per Mythryl convention, 'arg' will point to the second word,
	// so all we have to do is cast it appropriately:
	//
	struct barrier_struct*  barrier
	    =
	    *((struct barrier_struct**) arg);

	switch (barrier->state) {
	    //
	    case UNINITIALIZED_BARRIER:
	    case   INITIALIZED_BARRIER:
	    case       CLEARED_BARRIER:
		//
		barrier->state = FREED_BARRIER;
log_if("barrier_free free()'d barrier %x", barrier);
		free( barrier );
		break;

	    case         FREED_BARRIER:
		log_if("Attempt to free already-freed barrier instance.");
		die(   "Attempt to free already-freed barrier instance.");

	    default:
		log_if("barrier_free: Attempt to free bogus value. (Already-freed barrier? Junk?)");
		die(   "barrier_free: Attempt to free bogus value. (Already-freed barrier? Junk?)");
	}
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
	//
    #else
	die ("barrier_free: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   barrier_init   (Task* task,  Val arg)   {
    //       ============
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("barrier_init");

    #if NEED_PTHREAD_SUPPORT

	Val barrier_arg = GET_TUPLE_SLOT_AS_VAL(arg, 0);
	int threads	= GET_TUPLE_SLOT_AS_INT(arg, 1);

	struct barrier_struct*  barrier
	    =
	    *((struct barrier_struct**) barrier_arg);

	switch (barrier->state) {
	    //
	    case UNINITIALIZED_BARRIER:
	    case       CLEARED_BARRIER:
		//
		{   char* err =   pth__barrier_init( task, arg, &barrier->barrier, threads );
		    //
		    if (err)		{ log_if("pth__barrier_init returned err");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err , NULL);	}
		    else		{ barrier->state = INITIALIZED_BARRIER;		return HEAP_VOID;			}
		}
		break;

	    case   INITIALIZED_BARRIER:	log_if("barrier_init:  Attempt to set already-set barrier.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to set already-set barrier.", NULL);
	    case         FREED_BARRIER:	log_if("barrier_init:  Attempt to set freed barrier.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to set freed barrier.", NULL);
	    default:			log_if("barrier_init:  Attempt to set bogus value.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "barrier_init: Attempt to set bogus value. (Already-freed barrier? Junk?)", NULL);
	}
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.

    #else
	die ("barrier_init: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   barrier_destroy   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("barrier_destroy");

    #if NEED_PTHREAD_SUPPORT

	struct barrier_struct*  barrier
	    =
	    *((struct barrier_struct**) arg);

	switch (barrier->state) {
	    //
	    case   INITIALIZED_BARRIER:
		{
		    char* err =   pth__barrier_destroy( task, arg, &barrier->barrier );
		    //
		    if (err)	{ log_if("pth__barrier_destroy returned error.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
		    else	{ barrier->state = CLEARED_BARRIER;			return HEAP_VOID;			}
		}
		break;

	    case UNINITIALIZED_BARRIER:	log_if("barrier_destroy:  Attempt to clear uninitialized barrier");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to clear uninitialized barrier.", NULL);
	    case       CLEARED_BARRIER:	log_if("barrier_destroy:  Attempt to clear already-cleared barrier.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to clear already-cleared barrier.", NULL);
	    case         FREED_BARRIER:	log_if("barrier_destroy:  Attempt to clear already-freed barrier.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to clear already-freed barrier.", NULL);
	    default:			log_if("barrier_destroy:  Attempt to clear bogus value.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "barrier_destroy: Attempt to clear bogus value. (Already-freed barrier? Junk?)", NULL);
	}
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.

    #else
	die ("barrier_destroy: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   barrier_wait   (Task* task,  Val arg)   {
    //       ============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("barrier_wait");

    #if NEED_PTHREAD_SUPPORT

	struct barrier_struct*  barrier
	    =
	    *((struct barrier_struct**) arg);

	switch (barrier->state) {
	    //
	    case   INITIALIZED_BARRIER:
		{   Bool result;
		    char* err =   pth__barrier_wait( task, arg, &barrier->barrier, &result );
		    //
		    if (err)	{ log_if("barrier_wait returned error");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
		    if (result)							return HEAP_TRUE;
		    else							return HEAP_FALSE;
		}
		break;

	    case UNINITIALIZED_BARRIER:	log_if("barrier_wait: Attempt to wait on uninitialized barrier.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to wait on uninitialized barrier.", NULL);
	    case       CLEARED_BARRIER:	log_if("barrier_wait: Attempt to wait on cleared barrier."); 		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to wait on cleared barrier.", NULL);
	    case         FREED_BARRIER:	log_if("barrier_wait: Attempt to wait on freed barrier.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to wait on freed barrier.", NULL);
	    default:			log_if("barrier_wait: Attempt to wait on bogus value.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "barrier_wait: Attempt to wait on bogus value. (Already-freed barrier?)", NULL);
	}
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.

    #else
	die ("barrier_wait: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}



static Val   condvar_make   (Task* task,  Val arg)   {
    //       ============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("condvar_make");

    #if NEED_PTHREAD_SUPPORT
	//
	// We allocate the condvar_struct on the C
	// heap rather than the Mythryl heap because
	// having the garbage collector moving condvars
	// around in memory seems like a really, really
	// bad idea:
	//
	struct condvar_struct*  condvar
	    =
	    (struct condvar_struct*)  MALLOC( sizeof(struct condvar_struct) );	if (!condvar) die("Unable to malloc condvar_struct"); 

log_if("condvar_make: malloc()d condvar %x", condvar);
	condvar->state = UNINITIALIZED_CONDVAR;				// So we can catch attempts to wait on an uninitialized condvar at this level.

	// We return the address of the condvar_struct
	// to the Mythryl level encoded as a word value:
	//
        return  make_one_word_unt(task,  (Val_Sized_Unt)condvar  );

    #else
	die ("condvar_make: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   condvar_free   (Task* task,  Val arg)   {
    //       ============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("condvar_free");

    #if NEED_PTHREAD_SUPPORT
	// 'arg' should be something returned by barrier_make() above,
	// so it should be a Mythryl boxed word -- a two-word heap record
	// consisting of a tagword  MAKE_TAGWORD(1, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG)
	// followed by the C address of our   struct barrier_struct.
	// Per Mythryl convention, 'arg' will point to the second word,
	// so all we have to do is cast it appropriately:
	//
	struct condvar_struct*  condvar
	    =
	    *((struct condvar_struct**) arg);

	switch (condvar->state) {
	    //
	    case UNINITIALIZED_CONDVAR:
	    case   INITIALIZED_CONDVAR:
	    case       CLEARED_CONDVAR:
		//
		condvar->state = FREED_BARRIER;
log_if("condvar_free: free()d condvar %x", condvar);
		free( condvar );
		break;

	    case         FREED_CONDVAR:
		log_if("Attempt to free already-freed condvar instance.");
		die(   "Attempt to free already-freed condvar instance.");

	    default:
		log_if("condvar_free: Attempt to free bogus value. (Already-freed condvar? Junk?)");
		die(   "condvar_free: Attempt to free bogus value. (Already-freed condvar? Junk?)");
	}
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
	//
    #else
	die ("condvar_free: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   condvar_init   (Task* task,  Val arg)   {
    //       ============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("condvar_init");

    #if NEED_PTHREAD_SUPPORT

	struct condvar_struct*  condvar
	    =
	    *((struct condvar_struct**) arg);

	switch (condvar->state) {
	    //
	    case UNINITIALIZED_CONDVAR:
	    case       CLEARED_CONDVAR:
		{
		    char* err = pth__condvar_init( task, arg, &condvar->condvar );
		    //
		    if (err)		{ log_if("pth__condvar_init returned error");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
		    else		{ condvar->state = INITIALIZED_BARRIER;		return HEAP_VOID;			}
		}
		break;

	    case   INITIALIZED_CONDVAR:	log_if("condvar_init:  Attempt to set already-set condvar.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to set already-set condvar.", NULL);
	    case         FREED_CONDVAR:	log_if("condvar_init:  Attempt to set freed condvar.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to set freed condvar.", NULL);
	    default:			log_if("condvar_init:  Attempt to set bogus value.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "condvar_init: Attempt to set bogus value. (Already-freed condvar? Junk?)", NULL);
	}
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.

    #else
	die ("condvar_init: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   condvar_destroy   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("condvar_destroy");

    #if NEED_PTHREAD_SUPPORT

	struct condvar_struct*  condvar
	    =
	    *((struct condvar_struct**) arg);

	switch (condvar->state) {
	    //
	    case   INITIALIZED_CONDVAR:
		{
		    char* err =  pth__condvar_destroy( task, arg, &condvar->condvar );
		    //
		    if (err)		{ log_if("condvar_destroy returned error");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );		}
		    else		{ condvar->state = CLEARED_CONDVAR;		return HEAP_VOID;	  			}
		}
		break;

	    case UNINITIALIZED_CONDVAR:	log_if("condvar_destroy:  Attempt to clear uninitialized condvar.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to clear uninitialized condvar.", NULL);
	    case       CLEARED_CONDVAR:	log_if("condvar_destroy:  Attempt to clear already-cleared condvar.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to clear already-cleared condvar.", NULL);
	    case         FREED_CONDVAR:	log_if("condvar_destroy:  Attempt to clear already-freed condvar.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to clear already-freed condvar.", NULL);
	    default:			log_if("condvar_destroy:  Attempt to clear bogus value.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "condvar_destroy: Attempt to clear bogus value. (Already-freed condvar? Junk?)", NULL);
	}
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.

    #else
	die ("condvar_destroy: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   condvar_wait   (Task* task,  Val arg)   {
    //       ============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("condvar_wait");

if (running_script) log_if("condvar_wait: TOP...");
    #if NEED_PTHREAD_SUPPORT

	Val condvar_arg = GET_TUPLE_SLOT_AS_VAL(arg, 0);
	Val mutex_arg   = GET_TUPLE_SLOT_AS_VAL(arg, 1);

if (running_script) log_if("condvar_wait: AAA...");
	struct condvar_struct*  condvar =   *((struct condvar_struct**) condvar_arg);
	struct   mutex_struct*  mutex   =   *((struct   mutex_struct**)   mutex_arg);

if (running_script) log_if("condvar_wait: BBB...");
	switch (condvar->state) {
	    //
	    case   INITIALIZED_CONDVAR:		break;
	    //
	    case UNINITIALIZED_CONDVAR:		log_if("condvar_wait: Attempt to wait on uninitialized condvar.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to wait on uninitialized condvar.", NULL);
	    case       CLEARED_CONDVAR:		log_if("condvar_wait: Attempt to wait on already-cleared condvar.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to wait on already-cleared condvar.", NULL);
	    case         FREED_CONDVAR:		log_if("condvar_wait: Attempt to wait on already-freed condvar.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to wait on already-freed condvar.", NULL);
	    default:				log_if("condvar_wait: Attempt to wait on bogus value.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "condvar_wait: Attempt to wait on bogus value.", NULL);
	}

	switch (mutex->state) {
	    //
	    case   INITIALIZED_MUTEX:		break;
	    //
	    case UNINITIALIZED_MUTEX:		log_if("condvar_wait: Attempt to condvar_wait on uninitialized mutex."); return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to condvar_wait on uninitialized mutex.", NULL);
	    case       CLEARED_MUTEX:		log_if("condvar_wait: Attempt to condvar_wait on cleared mutex.");	 return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to condvar_wait on cleared mutex.", NULL);
	    case         FREED_MUTEX:		log_if("condvar_wait: Attempt to condvar_wait on freed condvar.");	 return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to condvar_wait on freed condvar.", NULL);
	    default:				log_if("condvar_wait: Attempt to convar_wait on bogus mutex value.");	 return RAISE_ERROR__MAY_HEAPCLEAN( task, "condvar_wait: Attempt to convar_wait on bogus mutex value.", NULL);
	}

	{   char* err =  pth__condvar_wait( task, arg, &condvar->condvar, &mutex->mutex );
	    //
	    if (err)		{					log_if("condvar_wait: condvar_wait returned error");			 return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
	    else		{														 return HEAP_VOID;			}
	}



    #else
	die ("condvar_wait: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   condvar_signal   (Task* task,  Val arg)   {
    //       ==============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("condvar_signal");

if (running_script) log_if("condvar_signal: TOP...");
    #if NEED_PTHREAD_SUPPORT

	struct condvar_struct*  condvar
	    =
	    *((struct condvar_struct**) arg);

	switch (condvar->state) {
	    //
	    case   INITIALIZED_CONDVAR:
		{
		    char* err =  pth__condvar_signal( task, arg, &condvar->condvar );
		    //
		    if (err)	{ log_if("condvar_signal returned error");		return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
		    else								return HEAP_VOID;
		}
		break;

	    case UNINITIALIZED_CONDVAR:	log_if("condvar_signal:  Attempt to signal via uninitialized condvar.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to signal via uninitialized condvar.", NULL);
	    case       CLEARED_CONDVAR:	log_if("condvar_signal:	 Attempt to signal via cleared condvar.");			return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to signal via cleared condvar.", NULL);
	    case         FREED_CONDVAR:	log_if("condvar_signal:	 Attempt to signal via freed condvar.");			return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to signal via freed condvar.", NULL);
	    default:			log_if("condvar_signal:	 Attempt to signal via bogus value.");				return RAISE_ERROR__MAY_HEAPCLEAN( task, "condvar_signal: Attempt to signal via bogus value.", NULL);
	}
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.

    #else
	die ("condvar_signal: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   condvar_broadcast   (Task* task,  Val arg)   {
    //       =================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("condvar_broadcast");

if (running_script) log_if("condvar_broadcast: TOP...");
    #if NEED_PTHREAD_SUPPORT

	struct condvar_struct*  condvar
	    =
	    *((struct condvar_struct**) arg);

if (running_script) log_if("condvar_broadcast: AAA...");
	switch (condvar->state) {
	    //
	    case   INITIALIZED_CONDVAR:
		{
if (running_script) log_if("condvar_broadcast: BBB...");
		    char* err =  pth__condvar_broadcast( task, arg, &condvar->condvar );
if (running_script) log_if("condvar_broadcast: CCC...");
		    //
		    if (err)			{ log_if("condvar_broadcast returned error.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
		    else			{						return HEAP_VOID;			}
		}
		break;

	    case UNINITIALIZED_CONDVAR:		log_if("condvar_broadcast: Attempt to broadcast via uninitialized condvar.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to broadcast via uninitialized condvar.", NULL);
	    case       CLEARED_CONDVAR:		log_if("condvar_broadcast: Attempt to broadcast via cleared condvar.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to broadcast via cleared condvar.", NULL);
	    case         FREED_CONDVAR:		log_if("condvar_broadcast: Attempt to broadcast via freed condvar.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to broadcast via freed condvar.", NULL);
	    default:				log_if("condvar_broadcast: Attempt to broadcast via bogus value.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "condvar_broadcast: Attempt to broadcast via bogus value.", NULL);
	}

    #else
	die ("condvar_broadcast: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}



static Mythryl_Name_With_C_Function CFunTable[] = {
    //
    { "get_pthread_id","get_pthread_id",	get_pthread_id,		""},
    { "spawn_pthread","spawn_pthread",		spawn_pthread,		""},
    { "join_pthread","join_pthread",		join_pthread,		""},
    { "pthread_exit","pthread_exit",		pthread_exit_fn,	""},
    //
    { "mutex_make","mutex_make",		mutex_make,		""},
    { "mutex_free","mutex_free",		mutex_free,		""},
    { "mutex_init","mutex_init",		mutex_init,		""},
    { "mutex_destroy","mutex_destroy",		mutex_destroy,		""},
    { "mutex_lock","mutex_lock",		mutex_lock,		""},
    { "mutex_unlock","mutex_unlock",		mutex_unlock,		""},
    { "mutex_trylock","mutex_trylock",		mutex_trylock,		""},
    //
    { "condvar_make","condvar_make",		condvar_make,		""},
    { "condvar_free","condvar_free",		condvar_free,		""},
    { "condvar_init","condvar_init",		condvar_init,		""},
    { "condvar_destroy","condvar_destroy",	condvar_destroy,	""},
    { "condvar_wait","condvar_wait",		condvar_wait,		""},
    { "condvar_signal","condvar_signal",	condvar_signal,		""},
    { "condvar_broadcast","condvar_broadcast",	condvar_broadcast,	""},
    //
    { "barrier_make","barrier_make",		barrier_make,		""},
    { "barrier_free","barrier_free",		barrier_free,		""},
    { "barrier_init","barrier_init",		barrier_init,		""},
    { "barrier_destroy","barrier_destroy",	barrier_destroy,	""},
    { "barrier_wait","barrier_wait",		barrier_wait,		""},
    //
    CFUNC_NULL_BIND
};


// The pthread (multicore support) library:
//
// Our record                Libmythryl_Pthread
// gets compiled into        src/c/lib/mythryl-callable-c-libraries-list.h
// and thus ultimately       mythryl_callable_c_libraries__local []
// in                        src/c/lib/mythryl-callable-c-libraries.c
//
Mythryl_Callable_C_Library	    Libmythryl_Pthread = {						// Mythryl_Callable_C_Library		def in    src/c/h/mythryl-callable-c-libraries.h
    //                              ==================
    "pthread",			// Library name.
    "1.0",			// Library version.
    "December 18, 1994",	// Library creation date.
    NULL,
    CFunTable			// Library functions.
};



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

