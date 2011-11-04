// libmythryl-pthread.c

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

// This file defines the "pthread" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my acquire_pthread:   (Thread, Fate) -> Bool
//         =
//         mythryl_callable_c_library_interface::find_c_function  { lib_name => "pthread", fun_name => "acquire_pthread" };
// 
// or such.

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

static Val   acquire_pthread   (Task* task,  Val arg)   {			// Apparently never called.
    //       ===============
    //
    #if NEED_PTHREAD_SUPPORT
	//
	return pth__acquire_pthread( task, arg );				// pth__acquire_pthread	def in    src/c/pthread/pthread-on-posix-threads.c
        //									// pth__acquire_pthread	def in    src/c/pthread/pthread-on-sgi.c
    #else									// pth__acquire_pthread	def in    src/c/pthread/pthread-on-solaris.c
	die ("acquire_pthread: no mp support\n");
        return HEAP_TRUE;							// Cannot execute; only present to quiet gcc.
    #endif
}



static Val release_pthread   (Task* task,  Val arg)   {
    //     ===============
    //
    #if NEED_PTHREAD_SUPPORT
	pth__release_pthread(task);  	// Should not return.
	die ("_lib7_MP_release_pthread: call unexpectedly returned\n");
    #else
	die ("_lib7_MP_release_pthread: no mp support\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}


static Val   max_pthreads   (Task* task,  Val arg)   {				// Apparently nowhere invoked.
    //       ============
    //
    #if NEED_PTHREAD_SUPPORT
	return TAGGED_INT_FROM_C_INT(pth__max_pthreads ());
    #else
	die ("_lib7_MP_max_pthreads: no mp support\n");
        return TAGGED_INT_FROM_C_INT( 0 );					// Cannot execute; only present to quiet gcc.
    #endif
}


static Val   spin_lock   (Task* task,  Val arg)   {
    //       =========
    //
    #if NEED_PTHREAD_SUPPORT
	// "This code is for use the assembly (MIPS.prim.asm) try_lock and lock"
        //         --- Original SML/NJ comment.
        //
        // 2010-11-30 CrT:
        //     'try_lock' is defined in
        //                     src/c/machine-dependent/prim.intel32.asm
	//                     src/c/machine-dependent/prim.sparc32.asm
        //                     src/c/machine-dependent/prim.pwrpc32.asm
        //                     src/c/machine-dependent/prim.intel32.masm 
        //     but function makes no obvious use of them.
        //     'try_lock is also published in RunVec in
        //                     src/c/main/construct-runtime-package.c
        //     -- possibly that route replaced this one, which bitrotted?
	Val             result;
	REF_ALLOC(task, result, HEAP_FALSE);						// REF_ALLOC	def in    src/c/h/make-strings-and-vectors-etc.h
	return          result;
    #else
	die ("lib7_spin_lock: no mp support\n");
        return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
    #endif
}

static Mythryl_Name_With_C_Function CFunTable[] = {
    //
    { "get_pthread_id","get_pthread_id",	get_pthread_id,		""},
    { "acquire_pthread","acquire_pthread",	acquire_pthread,	""},
    { "max_pthreads","max_pthreads",		max_pthreads,		""},
    { "release_pthread","release_pthread",	release_pthread,	""},
    { "spin_lock","spin_lock",			spin_lock,		""},
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

