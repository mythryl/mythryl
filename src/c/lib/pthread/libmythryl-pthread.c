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

    return TAGGED_INT_FROM_C_INT( pth__get_pthread_id());			// pth__get_pthread_id	def in    src/c/pthread/pthread-on-posix-threads.c
}

static Val   do_spawn_pthread   (Task* task,  Val closure)   {			// Apparently never called.
    //       ================
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("spawn_pthread");

    //
//  Val current_thread =  GET_TUPLE_SLOT_AS_VAL( arg, 0 );			// This is stored into   pthread->task->current_thread.   NB: "task->current_thread" was "task->ml_varReg" back when this was written -- CML came later.
//  Val closure        =  GET_TUPLE_SLOT_AS_VAL( arg, 1 );			// This is stored into   pthread->task->current_closure

    int pthread_table_slot;							// An index into the   pthread_table__global[]   defined in   src/c/main/runtime-state.c
    char* err = pth__pthread_create( &pthread_table_slot, task->current_thread, closure );

    if (err)   return RAISE_ERROR__MAY_HEAPCLEAN(task, err, NULL);
    else	   return TAGGED_INT_FROM_C_INT( pthread_table_slot );
									    // pth__pthread_create	def in    src/c/pthread/pthread-on-posix-threads.c
    //									// pth__pthread_create	def in    src/c/pthread/pthread-on-sgi.c
}



static Val   do_pthread_exit   (Task* task,  Val arg)   {			// Name issues: The name 'pthread_exit' is used by <pthread.h>, and of course 'exit' by <stdlib.h>.
    //       ===============
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("pthread_exit_fn");
    pth__pthread_exit(task);  	// Should not return.
    log_if("pthread_exit_fn: call unexpectedly returned\n");
    die(   "pthread_exit_fn: call unexpectedly returned\n");

    return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
}


static Val   do_join_pthread   (Task* task,  Val pthread_to_join)   {		// Name issue: 'pthread_join' is used by <pthread.h>
    //       ===============							// 'pthread_to_join' is a pthread_table__global[] index returned from a call to   spawn_pthread()   (above).
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("join_pthread");

    char* err = pth__pthread_join( task, pthread_to_join );			// Used to pass TAGGED_INT_TO_C_INT( pthread_to_join ) );
	//
    if (err) {  log_if("join_pthread failed!");		return RAISE_ERROR__MAY_HEAPCLEAN(task, err,NULL );	}
    else     {						return HEAP_VOID;			}
}



static Val do_mutex_make   (Task* task,  Val arg)   {
    //     =============
    //
										ENTER_MYTHRYL_CALLABLE_C_FN("do_mutex_make");

    // We allocate the mutex on the C heap rather
    // than the Mythryl heap because having the
    // garbage collector moving mutexes around in
    // memory seems like a really, really bad idea:				// In particular, the Linux implementation contains linklist pointers.
    //
    return TAGGED_INT_FROM_C_INT( pth__mutex_make() );

}

static Val   do_mutex_free   (Task* task,  Val arg)   {
    //       =============
    //
										ENTER_MYTHRYL_CALLABLE_C_FN("do_mutex_free");
    // 'arg' should be something returned by do_mutex_make():
    //
    {    char* err =  pth__mutex_destroy( task, TAGGED_INT_TO_C_INT( arg ) );
	//
	if (err)   return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );
    }

    return HEAP_VOID;
}

static Val   do_mutex_lock   (Task* task,  Val arg)   {
    //       =============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_mutex_lock");
    char* err =  pth__mutex_lock( task, TAGGED_INT_TO_C_INT( arg ) );
    //
    if (err)   return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );
    else       return HEAP_VOID;


    return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
}

static Val   do_mutex_unlock   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_mutex_unlock");
    char* err =  pth__mutex_unlock( task, TAGGED_INT_TO_C_INT( arg ) );
    //
    if (err)   return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );
    else       return HEAP_VOID;

    return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
}

static Val   do_mutex_trylock   (Task* task,  Val arg)   {
    //       ================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_mutex_trylock");
    Bool  result;
    char* err = pth__mutex_trylock( task, TAGGED_INT_TO_C_INT( arg ), &result );
    //
    if (err)		{	log_if("trylock returned error!");			return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
    //
    if (result)		{	log_if("trylock returning TRUE");			return HEAP_TRUE;			}	// Mutex was busy.
    else		{	log_if("trylock returning FALSE");			return HEAP_FALSE;			}	// Successfully acquired mutex.


    return HEAP_TRUE;							// Cannot execute.
}



static Val   do_barrier_make   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_barrier_make");
    // We allocate the barrier on the C heap
    // rather than the Mythryl heap because
    // having the garbage collector moving condvars
    // around in memory seems like a really, really
    // bad idea:
    //
    return TAGGED_INT_FROM_C_INT( pth__barrier_make() );
}

static Val   do_barrier_free   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_barrier_free");
    // 'arg' should be something returned by do_barrier_make():
    //
    char* err =  pth__barrier_free( task, TAGGED_INT_TO_C_INT( arg ) );
    //
    if (err)   return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );


    return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
}

static Val   do_barrier_init   (Task* task,  Val arg)   {
    //       ===============
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("do_barrier_init");
    Vunt barrier_id =  GET_TUPLE_SLOT_AS_INT(arg, 0);
    int  threads    =  GET_TUPLE_SLOT_AS_INT(arg, 1);

    char* err =   pth__barrier_init( task, barrier_id, threads );
    //
    if (err)		{ log_if("pth__barrier_init returned err");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err , NULL);	}
    else		{ 						return HEAP_VOID;			}

    return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
}

static Val   do_barrier_destroy   (Task* task,  Val arg)   {
    //       ==================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_barrier_destroy");
    char* err =   pth__barrier_destroy( task, TAGGED_INT_TO_C_INT( arg ) );
    //
    if (err)	{ log_if("pth__barrier_destroy returned error.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
    else	{ 							return HEAP_VOID;			}

    return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
}

static Val   do_barrier_wait   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_barrier_wait");
    Bool result;
    char* err =   pth__barrier_wait( task, TAGGED_INT_TO_C_INT( arg ), &result );
    //
    if (err)	{ log_if("do_barrier_wait returned error");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
    if (result)							return HEAP_TRUE;
    else							return HEAP_FALSE;

    return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
}



static Val   do_condvar_make   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_condvar_make");
    // We allocate the condvar_struct on the C
    // heap rather than the Mythryl heap because
    // having the garbage collector moving condvars
    // around in memory seems like a really, really
    // bad idea:
    //
    return TAGGED_INT_FROM_C_INT( pth__condvar_make() );
}

static Val   do_condvar_free   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_condvar_free");
    // 'arg' should be something returned by do_condvar_make():
    //
    {   char* err =  pth__condvar_destroy( task, TAGGED_INT_TO_C_INT( arg ) );
	//
	if (err)		{ log_if("do_condvar_destroy returned error");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );		}
	else		{ 						return HEAP_VOID;	  			}
    }
    return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
}

static Val   do_condvar_wait   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_condvar_wait");
    Val condvar_arg = GET_TUPLE_SLOT_AS_VAL(arg, 0);
    Val mutex_arg   = GET_TUPLE_SLOT_AS_VAL(arg, 1);

    char* err =  pth__condvar_wait( task,
                                    TAGGED_INT_TO_C_INT( condvar_arg ),
                                    TAGGED_INT_TO_C_INT(   mutex_arg )
                                  );
    //
    if (err)		{					log_if("do_condvar_wait: pth__condvar_wait returned error");		 return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
    else		{														 return HEAP_VOID;			}
}

static Val   do_condvar_signal   (Task* task,  Val arg)   {
    //       =================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_condvar_signal");
    char* err =  pth__condvar_signal( task, TAGGED_INT_TO_C_INT( arg ) );
    //
    if (err)	{ log_if("do_condvar_signal returned error");		return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
    else								return HEAP_VOID;

    return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
}

static Val   do_condvar_broadcast   (Task* task,  Val arg)   {
    //       ====================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN("do_condvar_broadcast");

    char* err =  pth__condvar_broadcast( task, TAGGED_INT_TO_C_INT( arg ) );
    //
    if (err)			{ log_if("do_condvar_broadcast returned error.");	return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
    else			{							return HEAP_VOID;			}

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

