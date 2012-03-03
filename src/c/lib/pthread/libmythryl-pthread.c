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



static Val   do_get_pthread_id         (Task* task,  Val arg)   {
    //       =================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("get_pthread_id");

    #if NEED_PTHREAD_SUPPORT
	//
        return TAGGED_INT_FROM_C_INT( pth__get_pthread_id());			// pth__get_pthread_id	def in    src/c/pthread/pthread-on-posix-threads.c
        //
    #else
	die ("get_pthread_id: no mp support\n");
        return TAGGED_INT_FROM_C_INT( 0 );					// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   do_spawn_pthread   (Task* task,  Val closure)   {			// Apparently never called.
    //       ================
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



static Val   do_pthread_exit   (Task* task,  Val arg)   {			// Name issues: The name 'pthread_exit' is used by <pthread.h>, and of course 'exit' by <stdlib.h>.
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


static Val   do_join_pthread   (Task* task,  Val pthread_to_join)   {		// Name issue: 'pthread_join' is used by <pthread.h>
    //       ===============							// 'pthread_to_join' is a pthread_table__global[] index returned from a call to   spawn_pthread()   (above).
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



static Val do_mutex_make   (Task* task,  Val arg)   {
    //     =============
    //
										ENTER_MYTHRYL_CALLABLE_C_FN("do_mutex_make");
    #if NEED_PTHREAD_SUPPORT

	// We allocate the mutex on the C heap rather
	// than the Mythryl heap because having the
	// garbage collector moving mutexes around in
	// memory seems like a really, really bad idea:				// In particular, the Linux implementation contains linklist pointers.
	//
	return TAGGED_INT_FROM_C_INT( pth__mutex_make() );

    #else

	die ("do_mutex_make: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   do_mutex_free   (Task* task,  Val arg)   {
    //       =============
    //

										ENTER_MYTHRYL_CALLABLE_C_FN("do_mutex_free");

    #if NEED_PTHREAD_SUPPORT

	// 'arg' should be something returned by barrier_make() above,
	// so it should be a Mythryl boxed word -- a two-word heap record
	// consisting of a tagword  MAKE_TAGWORD(1, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG)
	// followed by the mutex_vector__local index of our   Mutex.
	// Per Mythryl convention, 'arg' will point to the second word,
	// so all we have to do is cast it appropriately:
	//
	{    char* err =  pth__mutex_destroy( task, arg, TAGGED_INT_TO_C_INT( arg ) );
	    //
	    if (err)   return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );
	}

        return HEAP_VOID;
	//
    #else
	die ("do_mutex_free: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   do_mutex_lock   (Task* task,  Val arg)   {
    //       =============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_mutex_lock");

    #if NEED_PTHREAD_SUPPORT

	{   char* err =  pth__mutex_lock( task, arg, TAGGED_INT_TO_C_INT( arg ) );
	    //
	    if (err)   return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );
	    else       return HEAP_VOID;
	}

        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.

    #else
	die ("do_mutex_lock: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   do_mutex_unlock   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_mutex_unlock");

    #if NEED_PTHREAD_SUPPORT

	{   char* err =  pth__mutex_unlock( task, arg, TAGGED_INT_TO_C_INT( arg ) );
	    //
	    if (err)   return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );
	    else       return HEAP_VOID;
	}

        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.

    #else
	die ("do_mutex_unlock: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   do_mutex_trylock   (Task* task,  Val arg)   {
    //       ================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_mutex_trylock");

    #if NEED_PTHREAD_SUPPORT

	{   Bool  result;
	    char* err = pth__mutex_trylock( task, arg, TAGGED_INT_TO_C_INT( arg ), &result );
	    //
	    if (err)		{	log_if("trylock returned error!");			return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
	    //
	    if (result)		{	log_if("trylock returning TRUE");			return HEAP_TRUE;			}	// Mutex was busy.
	    else		{	log_if("trylock returning FALSE");			return HEAP_FALSE;			}	// Successfully acquired mutex.
	};

        return HEAP_TRUE;							// Cannot execute.

    #else
	die ("do_mutex_trylock: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}



static Val   do_barrier_make   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_barrier_make");

    #if NEED_PTHREAD_SUPPORT
	//
	// We allocate the condvar_struct on the C
	// heap rather than the Mythryl heap because
	// having the garbage collector moving condvars
	// around in memory seems like a really, really
	// bad idea:
	//
	struct barrier_struct*  barrier
	    =
	    (struct barrier_struct*)  MALLOC( sizeof(struct barrier_struct) );	if (!barrier) die("Unable to malloc barrier_struct"); 

log_if("do_barrier_make malloc()'d barrier %x", barrier);
	barrier->state = UNINITIALIZED_BARRIER;				// So we can catch attempts to wait on an uninitialized barrier at this level.

	// We return the address of the barrier_struct
	// to the Mythryl level encoded as a word value:
	//
        return  make_one_word_unt(task, (Val_Sized_Unt)barrier );

    #else
	die ("do_barrier_make: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   do_barrier_free   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_barrier_free");

   #if NEED_PTHREAD_SUPPORT
	// 'arg' should be something returned by do_barrier_make() above,
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
log_if("do_barrier_free free()'d barrier %x", barrier);
		free( barrier );
		break;

	    case         FREED_BARRIER:
		log_if("Attempt to free already-freed barrier instance.");
		die(   "Attempt to free already-freed barrier instance.");

	    default:
		log_if("do_barrier_free: Attempt to free bogus value. (Already-freed barrier? Junk?)");
		die(   "do_barrier_free: Attempt to free bogus value. (Already-freed barrier? Junk?)");
	}
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
	//
    #else
	die ("do_barrier_free: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   do_barrier_init   (Task* task,  Val arg)   {
    //       ===============
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_barrier_init");

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

	    case   INITIALIZED_BARRIER:	log_if("do_barrier_init:  Attempt to set already-set barrier.");return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to init already-set barrier.", NULL);
	    case         FREED_BARRIER:	log_if("do_barrier_init:  Attempt to set freed barrier.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to init freed barrier.", NULL);
	    default:			log_if("do_barrier_init:  Attempt to set bogus value.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to init bogus varrier value. (Already-freed barrier? Junk?)", NULL);
	}
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.

    #else
	die ("do_barrier_init: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   do_barrier_destroy   (Task* task,  Val arg)   {
    //       ==================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_barrier_destroy");

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

	    case UNINITIALIZED_BARRIER:	log_if("do_barrier_destroy:  Attempt to clear uninitialized barrier");	 return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to destroy uninitialized barrier.", NULL);
	    case       CLEARED_BARRIER:	log_if("do_barrier_destroy:  Attempt to clear already-cleared barrier.");return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to destroy already-cleared barrier.", NULL);
	    case         FREED_BARRIER:	log_if("do_barrier_destroy:  Attempt to clear already-freed barrier.");	 return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to destroy already-freed barrier.", NULL);
	    default:			log_if("do_barrier_destroy:  Attempt to clear bogus value.");		 return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to destroy bogus barrier value. (Already-freed barrier? Junk?)", NULL);
	}
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.

    #else
	die ("do_barrier_destroy: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   do_barrier_wait   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_barrier_wait");

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
		    if (err)	{ log_if("do_barrier_wait returned error");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
		    if (result)							return HEAP_TRUE;
		    else							return HEAP_FALSE;
		}
		break;

	    case UNINITIALIZED_BARRIER:	log_if("do_barrier_wait: Attempt to wait on uninitialized barrier.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to wait on uninitialized barrier.", NULL);
	    case       CLEARED_BARRIER:	log_if("do_barrier_wait: Attempt to wait on cleared barrier."); 	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to wait on cleared barrier.", NULL);
	    case         FREED_BARRIER:	log_if("do_barrier_wait: Attempt to wait on freed barrier.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to wait on freed barrier.", NULL);
	    default:			log_if("do_barrier_wait: Attempt to wait on bogus value.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to wait on bogus barrier value. (Already-freed barrier?)", NULL);
	}
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.

    #else
	die ("do_barrier_wait: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}



static Val   do_condvar_make   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_condvar_make");

    #if NEED_PTHREAD_SUPPORT
	//
	// We allocate the condvar_struct on the C
	// heap rather than the Mythryl heap because
	// having the garbage collector moving condvars
	// around in memory seems like a really, really
	// bad idea:
	//
#ifdef OLD
	struct condvar_struct*  condvar
	    =
	    (struct condvar_struct*)  MALLOC( sizeof(struct condvar_struct) );	if (!condvar) die("Unable to malloc condvar_struct"); 

	condvar->state = UNINITIALIZED_CONDVAR;				// So we can catch attempts to wait on an uninitialized condvar at this level.

	// We return the address of the condvar_struct
	// to the Mythryl level encoded as a word value:
	//
        return  make_one_word_unt(task,  (Val_Sized_Unt)condvar  );
#else
	return TAGGED_INT_FROM_C_INT( pth__condvar_make() );
#endif

    #else
	die ("do_condvar_make: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   do_condvar_free   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_condvar_free");

    #if NEED_PTHREAD_SUPPORT
	// 'arg' should be something returned by do_barrier_make() above,
	// so it should be a Mythryl boxed word -- a two-word heap record
	// consisting of a tagword  MAKE_TAGWORD(1, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG)
	// followed by the C address of our   struct barrier_struct.
	// Per Mythryl convention, 'arg' will point to the second word,
	// so all we have to do is cast it appropriately:
	//
#ifdef OLD
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
log_if("do_condvar_free: free()d condvar %x", condvar);
		free( condvar );
		break;

	    case         FREED_CONDVAR:
		log_if("Attempt to free already-freed condvar instance.");
		die(   "Attempt to free already-freed condvar instance.");

	    default:
		log_if("do_condvar_free: Attempt to free bogus value. (Already-freed condvar? Junk?)");
		die(   "do_condvar_free: Attempt to free bogus value. (Already-freed condvar? Junk?)");
	}
#else
	{   char* err =  pth__condvar_destroy( task, arg, TAGGED_INT_TO_C_INT( arg ) );
	    //
	    if (err)		{ log_if("do_condvar_destroy returned error");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );		}
	    else		{ 						return HEAP_VOID;	  			}
	}
#endif
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
	//
    #else
	die ("do_condvar_free: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

// static Val   do_condvar_init   (Task* task,  Val arg)   {
//     //       ===============
//     //
// 									    ENTER_MYTHRYL_CALLABLE_C_FN("do_condvar_init");
// 
//     #if NEED_PTHREAD_SUPPORT
// 
// 	struct condvar_struct*  condvar
// 	    =
// 	    *((struct condvar_struct**) arg);
// 
// 	switch (condvar->state) {
// 	    //
// 	    case UNINITIALIZED_CONDVAR:
// 	    case       CLEARED_CONDVAR:
// 		{
// 		    char* err = pth__condvar_init( task, arg, &condvar->condvar );
// 		    //
// 		    if (err)		{ log_if("pth__condvar_init returned error");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
// 		    else		{ condvar->state = INITIALIZED_BARRIER;		return HEAP_VOID;			}
// 		}
// 		break;
// 
// 	    case   INITIALIZED_CONDVAR:	log_if("do_condvar_init:  Attempt to set already-set condvar.");return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to set already-set condvar.", NULL);
// 	    case         FREED_CONDVAR:	log_if("do_condvar_init:  Attempt to set freed condvar.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to set freed condvar.", NULL);
// 	    default:			log_if("do_condvar_init:  Attempt to set bogus value.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "do_condvar_init: Attempt to set bogus value. (Already-freed condvar? Junk?)", NULL);
// 	}
//         return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
// 
//     #else
// 	die ("do_condvar_init: unimplemented\n");
//         return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
//     #endif
// }

// static Val   do_condvar_destroy   (Task* task,  Val arg)   {
//     //       ==================
//     //
// 									    ENTER_MYTHRYL_CALLABLE_C_FN("do_condvar_destroy");
// 
//     #if NEED_PTHREAD_SUPPORT
// 
// 	struct condvar_struct*  condvar
// 	    =
// 	    *((struct condvar_struct**) arg);
// 
// 	switch (condvar->state) {
// 	    //
// 	    case   INITIALIZED_CONDVAR:
// 		{
// 		    char* err =  pth__condvar_destroy( task, arg, &condvar->condvar );
// 		    //
// 		    if (err)		{ log_if("do_condvar_destroy returned error");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );		}
// 		    else		{ condvar->state = CLEARED_CONDVAR;		return HEAP_VOID;	  			}
// 		}
// 		break;
// 
// 	    case UNINITIALIZED_CONDVAR:	log_if("do_condvar_destroy:  Attempt to clear uninitialized condvar.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to clear uninitialized condvar.", NULL);
// 	    case       CLEARED_CONDVAR:	log_if("do_condvar_destroy:  Attempt to clear already-cleared condvar.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to clear already-cleared condvar.", NULL);
// 	    case         FREED_CONDVAR:	log_if("do_condvar_destroy:  Attempt to clear already-freed condvar.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to clear already-freed condvar.", NULL);
// 	    default:			log_if("do_condvar_destroy:  Attempt to clear bogus value.");			return RAISE_ERROR__MAY_HEAPCLEAN( task, "do_condvar_destroy: Attempt to clear bogus value.", NULL);
// 	}
//         return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
// 
//     #else
// 	die ("do_condvar_destroy: unimplemented\n");
//         return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
//     #endif
// }

static Val   do_condvar_wait   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_condvar_wait");

    #if NEED_PTHREAD_SUPPORT

	Val condvar_arg = GET_TUPLE_SLOT_AS_VAL(arg, 0);
	Val mutex_arg   = GET_TUPLE_SLOT_AS_VAL(arg, 1);

#ifdef OLD
	struct condvar_struct*  condvar =   *((struct condvar_struct**) condvar_arg);

	switch (condvar->state) {
	    //
	    case   INITIALIZED_CONDVAR:		break;
	    //
	    case UNINITIALIZED_CONDVAR:		log_if("do_condvar_wait: Attempt to wait on uninitialized condvar.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to wait on uninitialized condvar.", NULL);
	    case       CLEARED_CONDVAR:		log_if("do_condvar_wait: Attempt to wait on already-cleared condvar.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to wait on already-cleared condvar.", NULL);
	    case         FREED_CONDVAR:		log_if("do_condvar_wait: Attempt to wait on already-freed condvar.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to wait on already-freed condvar.", NULL);
	    default:				log_if("do_condvar_wait: Attempt to wait on bogus value.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "condvar_wait: Attempt to wait on bogus value.", NULL);
	}

	{   char* err =  pth__condvar_wait( task, arg, &condvar->condvar, TAGGED_INT_TO_C_INT( mutex_arg ) );
	    //
	    if (err)		{					log_if("do_condvar_wait: pth__condvar_wait returned error");		 return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
	    else		{														 return HEAP_VOID;			}
	}
#else
	{   char* err =  pth__condvar_wait( task, arg, TAGGED_INT_TO_C_INT( condvar_arg ), TAGGED_INT_TO_C_INT( mutex_arg ) );
	    //
	    if (err)		{					log_if("do_condvar_wait: pth__condvar_wait returned error");		 return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
	    else		{														 return HEAP_VOID;			}
	}
#endif

    #else
	die ("do_condvar_wait: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   do_condvar_signal   (Task* task,  Val arg)   {
    //       =================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_condvar_signal");

    #if NEED_PTHREAD_SUPPORT

#ifdef OLD
	struct condvar_struct*  condvar
	    =
	    *((struct condvar_struct**) arg);

	switch (condvar->state) {
	    //
	    case   INITIALIZED_CONDVAR:
		{
		    char* err =  pth__condvar_signal( task, arg, &condvar->condvar );
		    //
		    if (err)	{ log_if("do_condvar_signal returned error");		return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
		    else								return HEAP_VOID;
		}
		break;

	    case UNINITIALIZED_CONDVAR:	log_if("do_condvar_signal: Attempt to signal via uninitialized condvar.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to signal via uninitialized condvar.", NULL);
	    case       CLEARED_CONDVAR:	log_if("do_condvar_signal: Attempt to signal via cleared condvar.");			return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to signal via cleared condvar.", NULL);
	    case         FREED_CONDVAR:	log_if("do_condvar_signal: Attempt to signal via freed condvar.");			return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to signal via freed condvar.", NULL);
	    default:			log_if("do_condvar_signal: Attempt to signal via bogus value.");			return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to signal via bogus condvar value.", NULL);
	}
#else
	{   char* err =  pth__condvar_signal( task, arg, TAGGED_INT_TO_C_INT( arg ) );
	    //
	    if (err)	{ log_if("do_condvar_signal returned error");		return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
	    else								return HEAP_VOID;
	}
#endif
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.

    #else
	die ("do_condvar_signal: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val   do_condvar_broadcast   (Task* task,  Val arg)   {
    //       ====================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_condvar_broadcast");

    #if NEED_PTHREAD_SUPPORT

#ifdef OLD
	struct condvar_struct*  condvar
	    =
	    *((struct condvar_struct**) arg);

	switch (condvar->state) {
	    //
	    case   INITIALIZED_CONDVAR:
		{
		    char* err =  pth__condvar_broadcast( task, arg, &condvar->condvar );
		    //
		    if (err)			{ log_if("do_condvar_broadcast returned error.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
		    else			{							return HEAP_VOID;			}
		}
		break;

	    case UNINITIALIZED_CONDVAR:		log_if("do_condvar_broadcast: Attempt to broadcast via uninitialized condvar.");return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to broadcast via uninitialized condvar.", NULL);
	    case       CLEARED_CONDVAR:		log_if("do_condvar_broadcast: Attempt to broadcast via cleared condvar.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to broadcast via cleared condvar.", NULL);
	    case         FREED_CONDVAR:		log_if("do_condvar_broadcast: Attempt to broadcast via freed condvar.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to broadcast via freed condvar.", NULL);
	    default:				log_if("do_condvar_broadcast: Attempt to broadcast via bogus value.");		return RAISE_ERROR__MAY_HEAPCLEAN( task, "Attempt to broadcast via bogus condvar value.", NULL);
	}
#else
	{   char* err =  pth__condvar_broadcast( task, arg, TAGGED_INT_TO_C_INT( arg ) );
	    //
	    if (err)			{ log_if("do_condvar_broadcast returned error.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
	    else			{							return HEAP_VOID;			}
	}
#endif

    #else
	die ("do_condvar_broadcast: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}



static Mythryl_Name_With_C_Function CFunTable[] = {
    //
    { "get_pthread_id","get_pthread_id",	do_get_pthread_id,	""},
    { "spawn_pthread","spawn_pthread",		do_spawn_pthread,	""},
    { "join_pthread","join_pthread",		do_join_pthread,	""},
    { "pthread_exit","pthread_exit",		do_pthread_exit,	""},
    //
    { "mutex_make","mutex_make",		do_mutex_make,		""},
    { "mutex_free","mutex_free",		do_mutex_free,		""},
    { "mutex_lock","mutex_lock",		do_mutex_lock,		""},
    { "mutex_unlock","mutex_unlock",		do_mutex_unlock,	""},
    { "mutex_trylock","mutex_trylock",		do_mutex_trylock,	""},
    //
    { "condvar_make","condvar_make",		do_condvar_make,	""},
    { "condvar_free","condvar_free",		do_condvar_free,	""},
//    { "condvar_init","condvar_init",		do_condvar_init,	""},
//    { "condvar_destroy","condvar_destroy",	do_condvar_destroy,	""},
    { "condvar_wait","condvar_wait",		do_condvar_wait,	""},
    { "condvar_signal","condvar_signal",	do_condvar_signal,	""},
    { "condvar_broadcast","condvar_broadcast",	do_condvar_broadcast,	""},
    //
    { "barrier_make","barrier_make",		do_barrier_make,	""},
    { "barrier_free","barrier_free",		do_barrier_free,	""},
    { "barrier_init","barrier_init",		do_barrier_init,	""},
    { "barrier_destroy","barrier_destroy",	do_barrier_destroy,	""},
    { "barrier_wait","barrier_wait",		do_barrier_wait,	""},
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

