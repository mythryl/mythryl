// libmythryl-pthread.c
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
// some of the functionality defined in
//
//     src/c/h/runtime-pthread.h
//
// and implemented in the platform-specific files
//
//     src/c/pthread/pthread-on-posix-threads.c
//     src/c/pthread/pthread-on-sgi.c
//     src/c/pthread/pthread-on-solaris.c
//
// For background see:
//
//     src/A.PTHREAD-SUPPORT.OVERVIEW


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-pthread.h"
#include "runtime-values.h"
#include "runtime-globals.h"

#include "make-strings-and-vectors-etc.h"
#include "mythryl-callable-c-libraries.h"


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

struct mutex_struct {
    //
    int					padding0[ CACHE_LINE_BYTESIZE / sizeof(int) ];
    Mutex    mutex;
    int					padding1[ CACHE_LINE_BYTESIZE / sizeof(int)  ];
};

struct condvar_struct {
    //
    int					padding0[ CACHE_LINE_BYTESIZE / sizeof(int)  ];
    Condvar  condvar;
    int					padding1[ CACHE_LINE_BYTESIZE / sizeof(int)  ];
};

struct barrier_struct {
    //
    int					padding0[ CACHE_LINE_BYTESIZE / sizeof(int)  ];
    Barrier  barrier;
    int					padding1[ CACHE_LINE_BYTESIZE / sizeof(int)  ];
};


static Val   get_pthread_id         (Task* task,  Val arg)   {
// #if commented out because I want to test this individually without enabling the entire MP codebase -- 2011-10-30 CrT
//    #if NEED_PTHREAD_SUPPORT
	//
        return TAGGED_INT_FROM_C_INT( pth__get_pthread_id() );			// thread_id	def in    src/c/pthread/pthread-on-posix-threads.c
        //									// thread_id	def in    src/c/pthread/pthread-on-sgi.c
//    #else									// thread_id	def in    src/c/pthread/pthread-on-solaris.c
//	die ("get_pthread_id: no mp support\n");
//        return TAGGED_INT_FROM_C_INT( 0 );					// Cannot execute; only present to quiet gcc.
//    #endif
}

static Val   spawn_pthread   (Task* task,  Val closure)   {			// Apparently never called.
    //       =============
    //
    #if NEED_PTHREAD_SUPPORT
	//
//      Val current_thread =  GET_TUPLE_SLOT_AS_VAL( arg, 0 );			// This is stored into   pthread->task->current_thread.   NB: "task->current_thread" was "task->ml_varReg" back when this was written -- CML came later.
//      Val closure        =  GET_TUPLE_SLOT_AS_VAL( arg, 1 );			// This is stored into   pthread->task->current_closure

	return pth__pthread_create( task, task->current_thread, closure );	// pth__pthread_create	def in    src/c/pthread/pthread-on-posix-threads.c
        //									// pth__pthread_create	def in    src/c/pthread/pthread-on-sgi.c
    #else									// pth__pthread_create	def in    src/c/pthread/pthread-on-solaris.c
	die ("spawn_pthread: no mp support\n");
        return HEAP_TRUE;							// Cannot execute; only present to quiet gcc.
    #endif
}



static Val pthread_exit_fn   (Task* task,  Val arg)   {				// Name issues: 'pthread_exit' is used by <pthread.h>, and of course 'exit' by <stdlib.h>.
    //     ===============
    //
    #if NEED_PTHREAD_SUPPORT
	pth__pthread_exit(task);  	// Should not return.
	die ("pthread_exit_fn: call unexpectedly returned\n");
    #else
	die ("pthread_exit_fn: no mp support\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}



static Val mutex_make   (Task* task,  Val arg)   {
    //     ==========
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("mutex_make: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val mutex_free   (Task* task,  Val arg)   {
    //     ==========
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("mutex_free: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val mutex_init   (Task* task,  Val arg)   {
    //     ==========
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("mutex_init: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val mutex_destroy   (Task* task,  Val arg)   {
    //     =============
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("mutex_destroy: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val mutex_lock   (Task* task,  Val arg)   {
    //     ==========
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("mutex_lock: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val mutex_unlock   (Task* task,  Val arg)   {
    //     ============
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("mutex_unlock: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val mutex_trylock   (Task* task,  Val arg)   {
    //     =============
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("mutex_trylock: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}



static Val barrier_make   (Task* task,  Val arg)   {
    //     ============
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("barrier_make: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val barrier_free   (Task* task,  Val arg)   {
    //     ============
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("barrier_free: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val barrier_init   (Task* task,  Val arg)   {
    //     ============
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("barrier_init: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val barrier_destroy   (Task* task,  Val arg)   {
    //     ===============
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("barrier_destroy: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val barrier_wait   (Task* task,  Val arg)   {
    //     ============
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("barrier_wait: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}



static Val condvar_make   (Task* task,  Val arg)   {
    //     ============
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("condvar_make: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val condvar_free   (Task* task,  Val arg)   {
    //     ============
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("condvar_free: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val condvar_init   (Task* task,  Val arg)   {
    //     ============
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("condvar_init: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val condvar_destroy   (Task* task,  Val arg)   {
    //     ===============
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("condvar_destroy: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val condvar_wait   (Task* task,  Val arg)   {
    //     ============
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("condvar_wait: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val condvar_signal   (Task* task,  Val arg)   {
    //     ==============
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("condvar_signal: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Val condvar_broadcast   (Task* task,  Val arg)   {
    //     =================
    //
    #if NEED_PTHREAD_SUPPORT
    #else
	die ("condvar_broadcast: unimplemented\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}



static Mythryl_Name_With_C_Function CFunTable[] = {
    //
    { "get_pthread_id","get_pthread_id",	get_pthread_id,		""},
    { "spawn_pthread","spawn_pthread",		spawn_pthread,		""},
    { "pthread_exit","release_pthread",		pthread_exit_fn,	""},
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

