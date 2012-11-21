// libmythryl-hostthread.c
//
// For background see
//
//     src/A.HOSTTHREAD-SUPPORT.OVERVIEW
//
// and the docs at the bottom of
// 
//     src/c/hostthread/hostthread-on-posix-threads.c
//
//
//
// This file defines the "hostthread" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my spawn_hostthread:   Fate -> Bool
//         =
//         mythryl_callable_c_library_interface::find_c_function  { lib_name => "hostthread", fun_name => "spawn_hostthread" };
// 
// or such.

// Here we export to
//
//     src/lib/std/src/hostthread.api
//     src/lib/std/src/hostthread.pkg
//
// some of the functionality defined in the hostthread section of
//
//     src/c/h/runtime-base.h
//
// and implemented in the platform-specific file
//
//     src/c/hostthread/hostthread-on-posix-threads.c



/*
### 			"Programs are forests of fatally poison ivy through which paths have been beaten; 
### 			 we peek off the path only to see our forebears' skeletons leering back at us."
###
###									-- Hue White  
*/ 



#include "../../mythryl-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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



static Val   do_get_hostthread_name         (Task* task,  Val arg)   {	// 'arg' identifies hostthread; get name of that hostthread.
    //       ======================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
    int          i          =  TAGGED_INT_TO_C_INT( arg );
    Hostthread*  hostthread =  hostthread_table__global[ i ];
    char*        name       =  hostthread->name;
    Val          result     =  make_ascii_string_from_c_string__may_heapclean(task,  name, NULL);

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

static Val   do_set_hostthread_name         (Task* task,  Val arg)   {	// 'arg' is the name; set it on current hostthread.
    //       ======================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Hostthread*  hostthread =  pth__get_hostthread_by_ptid( pth__get_hostthread_ptid() );

    char*        name       =  HEAP_STRING_AS_C_STRING( arg );
    int          len        =  strlen( name );
    char*        dup        =  (char*) malloc( len + 1 );			// '+ 1' for terminal nul.

    strcpy( dup, name );							// Copy name from Mythryl heap to C heap.

    hostthread->name = dup;

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return HEAP_VOID;
}

static Val   do_get_hostthread_ptid         (Task* task,  Val arg)   {
    //       ======================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);


    Val result = make_one_word_int( task, (Vint) (pth__get_hostthread_ptid()));		// pth__get_hostthread_ptid	def in    src/c/hostthread/hostthread-on-posix-threads.c

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

static Val   do_get_hostthread_id         (Task* task,  Val arg)   {
    //       ====================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);


    Val result = TAGGED_INT_FROM_C_INT( pth__get_hostthread_id() );		// pth__get_hostthread_id	def in    src/c/hostthread/hostthread-on-posix-threads.c

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

static Val   do_spawn_hostthread   (Task* task,  Val closure)   {			// Apparently never called.
    //       ===================
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    //
//  Val current_thread =  GET_TUPLE_SLOT_AS_VAL( arg, 0 );			// This is stored into   hostthread->task->current_thread.   NB: "task->current_thread" was "task->ml_varReg" back when this was written -- CML came later.
//  Val closure        =  GET_TUPLE_SLOT_AS_VAL( arg, 1 );			// This is stored into   hostthread->task->current_closure

    int hostthread_table_slot;							// An index into the   hostthread_table__global[]   defined in   src/c/main/runtime-state.c
    char* err = pth__pthread_create( &hostthread_table_slot, task->current_thread, closure );

    Val result;

    if (err)	result = RAISE_ERROR__MAY_HEAPCLEAN(task, err, NULL);
    else	result = TAGGED_INT_FROM_C_INT( hostthread_table_slot );
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);

    return result;
										// pth__pthread_create	def in    src/c/hostthread/hostthread-on-posix-threads.c
    //										// pth__pthread_create	def in    src/c/hostthread/hostthread-on-sgi.c
}



static Val   do_hostthread_exit   (Task* task,  Val arg)   {			// 
    //       ==================
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
    pth__pthread_exit(task);  	// Should not return.
    log_if("pth__pthread_exit: call unexpectedly returned\n");
    die(   "pth__pthread_exit: call unexpectedly returned\n");
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);

    return HEAP_VOID;							// Cannot execute; only present to quiet gcc.
}


static Val   do_join_hostthread   (Task* task,  Val hostthread_to_join)   {	// 'hostthread_to_join' is a hostthread_table__global[] index returned from a call to   spawn_hostthread()   (above).
    //       ==================							
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    char* err = pth__pthread_join( task, hostthread_to_join );			// Used to pass TAGGED_INT_TO_C_INT( hostthread_to_join ) );
	//
    Val result;

    if (err) {  log_if("join_hostthread failed!");		return RAISE_ERROR__MAY_HEAPCLEAN(task, err,NULL );	}
    else     {							return HEAP_VOID;			}

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}



static Val do_mutex_make   (Task* task,  Val arg)   {
    //     =============
    //
										ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    // We allocate the mutex on the C heap rather
    // than the Mythryl heap because having the
    // garbage collector moving mutexes around in
    // memory seems like a really, really bad idea:				// In particular, the Linux implementation contains linklist pointers.
    //
    Val result = TAGGED_INT_FROM_C_INT( pth__mutex_make() );
										EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

static Val   do_mutex_free   (Task* task,  Val arg)   {
    //       =============
    //
										ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
    // 'arg' should be something returned by do_mutex_make():
    //
    char* err =  pth__mutex_destroy( task, TAGGED_INT_TO_C_INT( arg ) );
    //
    if (err)   return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );
										EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return HEAP_VOID;
}

static Val   do_mutex_lock   (Task* task,  Val arg)   {
    //       =============
    //
// Commented out 2012-10-28 because they were flooding the syscall log, hiding what I wanted to see:
// XXX SUCKO RESTOREME							    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
if (TAGGED_INT_TO_C_INT( arg ) == 6) ramlog_printf("do_mutex_lock(6)/AAA\n");
    char* err =  pth__mutex_lock( task, TAGGED_INT_TO_C_INT( arg ) );
    //
    Val result;

    if (err)   result = RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );
    else       result = HEAP_VOID;
if (TAGGED_INT_TO_C_INT( arg ) == 6) ramlog_printf("do_mutex_lock(6)/ZZZ\n");
// XXX SUCKO RESTOREME							    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

static Val   do_mutex_unlock   (Task* task,  Val arg)   {
    //       ===============
    //
// Commented out 2012-10-28 because they were flooding the syscall log, hiding what I wanted to see:
// XXX SUCKO RESTOREME							    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
if (TAGGED_INT_TO_C_INT( arg ) == 6) ramlog_printf("do_mutex_unlock(6)/AAA\n");
    char* err =  pth__mutex_unlock( task, TAGGED_INT_TO_C_INT( arg ) );
    //
    Val result;

    if (err)   result = RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );
    else       result = HEAP_VOID;
if (TAGGED_INT_TO_C_INT( arg ) == 6) ramlog_printf("do_mutex_unlock(6)/ZZZ\n");
// XXX SUCKO RESTOREME							    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

static Val   do_mutex_trylock   (Task* task,  Val arg)   {
    //       ================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
    Bool  flag;
    char* err = pth__mutex_trylock( task, TAGGED_INT_TO_C_INT( arg ), &flag );
    //
    Val result;

    if (err)		{	log_if("trylock returned error!");			result = RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
    else {
	//
	if (flag)	{	log_if("trylock returning TRUE");			result = HEAP_TRUE;			}	// Mutex was busy.
	else		{	log_if("trylock returning FALSE");			result = HEAP_FALSE;			}	// Successfully acquired mutex.
    }
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}



static Val   do_barrier_make   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
    // We allocate the barrier on the C heap
    // rather than the Mythryl heap because
    // having the garbage collector moving condvars
    // around in memory seems like a really, really
    // bad idea:
    //
    Val result = TAGGED_INT_FROM_C_INT( pth__barrier_make() );
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

static Val   do_barrier_free   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
    // 'arg' should be something returned by do_barrier_make():
    //
    char* err =  pth__barrier_free( task, TAGGED_INT_TO_C_INT( arg ) );
    //
    if (err)   return RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return HEAP_VOID;
}

static Val   do_barrier_init   (Task* task,  Val arg)   {
    //       ===============
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
    Vunt barrier_id =  GET_TUPLE_SLOT_AS_INT(arg, 0);
    int  threads    =  GET_TUPLE_SLOT_AS_INT(arg, 1);

    Val  result;

    char* err =   pth__barrier_init( task, barrier_id, threads );
    //
    if (err)		{ log_if("pth__barrier_init returned err");	result = RAISE_ERROR__MAY_HEAPCLEAN( task, err , NULL);	}
    else		{ 						result = HEAP_VOID;			}

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

static Val   do_barrier_wait   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
    Bool flag;
    char* err =   pth__barrier_wait( task, TAGGED_INT_TO_C_INT( arg ), &flag );
    //
    Val result;

    if (err)	{ log_if("do_barrier_wait returned error");	result = RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
    else {
	if (flag)						result = HEAP_TRUE;
	else							result = HEAP_FALSE;
    }
    return result;
}



static Val   do_condvar_make   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
    // We allocate the condvar_struct on the C
    // heap rather than the Mythryl heap because
    // having the garbage collector moving condvars
    // around in memory seems like a really, really
    // bad idea:
    //
    Val result = TAGGED_INT_FROM_C_INT( pth__condvar_make() );
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

static Val   do_condvar_free   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
    // 'arg' should be something returned by do_condvar_make():
    //
    char* err =  pth__condvar_destroy( task, TAGGED_INT_TO_C_INT( arg ) );
    //
    Val result;

    if (err)		{ log_if("do_condvar_destroy returned error");	result = RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );		}
    else		{ 						result = HEAP_VOID;	  			}

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

static Val   do_condvar_wait   (Task* task,  Val arg)   {
    //       ===============
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
    Val condvar_arg = GET_TUPLE_SLOT_AS_VAL(arg, 0);
    Val mutex_arg   = GET_TUPLE_SLOT_AS_VAL(arg, 1);

    char* err =  pth__condvar_wait( task,
                                    TAGGED_INT_TO_C_INT( condvar_arg ),
                                    TAGGED_INT_TO_C_INT(   mutex_arg )
                                  );
    Val result;
    //
    if (err)		{					log_if("do_condvar_wait: pth__condvar_wait returned error");		 result = RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
    else		{														 result = HEAP_VOID;			}

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

static Val   do_condvar_signal   (Task* task,  Val arg)   {
    //       =================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
    char* err =  pth__condvar_signal( task, TAGGED_INT_TO_C_INT( arg ) );
    //
    Val result;

    if (err)	{ log_if("do_condvar_signal returned error");		result = RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
    else								result = HEAP_VOID;

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

static Val   do_condvar_broadcast   (Task* task,  Val arg)   {
    //       ====================
    //
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    char* err =  pth__condvar_broadcast( task, TAGGED_INT_TO_C_INT( arg ) );
    //
    Val result;

    if (err)			{ log_if("do_condvar_broadcast returned error.");	result = RAISE_ERROR__MAY_HEAPCLEAN( task, err, NULL );	}
    else			{							result = HEAP_VOID;			}

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}



static Mythryl_Name_With_C_Function CFunTable[] = {
    //
    { "get_hostthread_name","get_hostthread_name",	do_get_hostthread_name,	""},
    { "set_hostthread_name","set_hostthread_name",	do_set_hostthread_name,	""},
    { "get_hostthread_ptid","get_hostthread_ptid",	do_get_hostthread_ptid,	""},
    { "get_hostthread_id",  "get_hostthread_id",	do_get_hostthread_id,	""},
    { "spawn_hostthread","spawn_hostthread",		do_spawn_hostthread,	""},
    { "join_hostthread","join_hostthread",		do_join_hostthread,	""},
    { "hostthread_exit","hostthread_exit",		do_hostthread_exit,	""},
    //
    { "mutex_make","mutex_make",			do_mutex_make,		""},
    { "mutex_free","mutex_free",			do_mutex_free,		""},
    { "mutex_lock","mutex_lock",			do_mutex_lock,		""},
    { "mutex_unlock","mutex_unlock",			do_mutex_unlock,	""},
    { "mutex_trylock","mutex_trylock",			do_mutex_trylock,	""},
    //
    { "condvar_make","condvar_make",			do_condvar_make,	""},
    { "condvar_free","condvar_free",			do_condvar_free,	""},
    { "condvar_wait","condvar_wait",			do_condvar_wait,	""},
    { "condvar_signal","condvar_signal",		do_condvar_signal,	""},
    { "condvar_broadcast","condvar_broadcast",		do_condvar_broadcast,	""},
    //
    { "barrier_make","barrier_make",			do_barrier_make,	""},
    { "barrier_free","barrier_free",			do_barrier_free,	""},
    { "barrier_init","barrier_init",			do_barrier_init,	""},
    { "barrier_wait","barrier_wait",			do_barrier_wait,	""},
    //
    CFUNC_NULL_BIND
};


// The hostthread (multicore support) library:
//
// Our record                Libmythryl_Hostthread
// gets compiled into        src/c/lib/mythryl-callable-c-libraries-list.h
// and thus ultimately       mythryl_callable_c_libraries__local []
// in                        src/c/lib/mythryl-callable-c-libraries.c
//
Mythryl_Callable_C_Library	    Libmythryl_Hostthread = {						// Mythryl_Callable_C_Library		def in    src/c/h/mythryl-callable-c-libraries.h
    //                              ==================
    "pthread",			// Library name.							// Should be renamed to "hostthread".
    "1.0",			// Library version.
    "December 18, 1994",	// Library creation date.
    NULL,
    CFunTable			// Library functions.
};



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

