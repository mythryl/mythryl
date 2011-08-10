// run-mythryl-code-and-runtime-eventloop.c

#include "../config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "runtime-base.h"
#include "runtime-configuration.h"
#include "runtime-values.h"
#include "pthread.h"
#include "task.h"
#include "heap-tags.h"
#include "asm-to-c-request-codes.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-globals.h"
#include "system-dependent-signal-stuff.h"
#include "mythryl-callable-c-libraries.h"
#include "profiler-call-counts.h"
#include "cleaner.h"

// Set up the return linkage and fate
// throwing in the Mythryl state vector.
//
#define SET_UP_RETURN( _task )	{							\
	    Task* __task    = (_task);							\
	    __task->closure	  = HEAP_VOID;						\
	    __task->program_counter = __task->fate;					\
	}

#define SET_UP_THROW( _task, _fate, _val )	{					\
	    Task* __task	 = (_task);						\
	    Val	__fate	         = (_fate);						\
	    __task->closure	 = __fate;						\
	    __task->fate         = HEAP_VOID;						\
	    __task->program_counter=							\
	    __task->link_register  = GET_CODE_ADDRESS_FROM_CLOSURE( __fate );		\
	    __task->exception_fate = HEAP_VOID;						\
	    __task->argument	 = (_val);						\
	}



Val   run_mythryl_function   (Task* task,  Val function,  Val argument,  Bool use_fate)   {
    //====================
    //
    // Apply the Mythryl closure 'function' to 'argument' and return the result.
    // If the flag   use_fate   is set, then the Task has already
    // been initialized with a return fate by save_c_state.

    initialize_task( task );								// initialize_task				def in   src/c/main/runtime-state.c

    // Initialize the calling context:
    //
    task->exception_fate =  PTR_CAST( Val,  handle_uncaught_exception_closure_v +1 );
    task->thread         =  HEAP_VOID;
    task->argument	 =  argument;

    if (!use_fate)     task->fate = PTR_CAST( Val,  return_to_c_level_c );		// See   ASM_CONT(return_to_c_level);   in   src/c/main/construct-runtime-package.c

    task->closure	   = function;

    task->program_counter  =
    task->link_register	   = GET_CODE_ADDRESS_FROM_CLOSURE( function );

    run_mythryl_task_and_runtime_eventloop( task );					// run_mythryl_task_and_runtime_eventloop	def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c

    return task->argument;
}


extern int  asm_run_mythryl_task  (Task* task);						// asm_run_mythryl_task	def in   src/c/machine-dependent/prim.intel32.asm
											// asm_run_mythryl_task	def in   src/c/machine-dependent/prim.pwrpc32.asm
											// asm_run_mythryl_task	def in   src/c/machine-dependent/prim.sparc32.asm
											//
											// asm_run_mythryl_task	def in   src/c/machine-dependent/win32-fault.c

// run_mythryl_task_and_runtime_eventloop
//
#if !defined(__CYGWIN32__)

void   run_mythryl_task_and_runtime_eventloop   (Task* task)   {
    // ======================================
    //
    // Called from:
    //     src/c/main/load-and-run-heap-image.c
    //     src/c/lib/ccalls/ccalls-fns.c

#else

void   system_run_mythryl_task_and_runtime_eventloop   (Task *task)   {				// called from			 src/c/machine-dependent/cygwin-fault.c
//     =============================================

#endif

    int		request;

    Pthread* pthread  =  task->pthread;

    Val	previous_profile_index =  PROF_OTHER;

    for (;;) {

        //     THIS_FN_PROFILING_HOOK_REFCELL_GLOBAL
        // is #defined in
        //     src/c/h/runtime-globals.h
        // in terms of
        //     this_fn_profiling_hook_refcell_global
        // from
        //     src/c/main/construct-runtime-package.c


	//     PROF_RUNTIME
	// is #defined in
	//     src/c/h/profiler-call-counts.h

	ASSIGN( THIS_FN_PROFILING_HOOK_REFCELL_GLOBAL, previous_profile_index );

	request = asm_run_mythryl_task( task );						//      asm_run_mythryl_task		def in    src/c/machine-dependent/prim.intel32.asm
											//      asm_run_mythryl_task		def in    src/c/machine-dependent/win32-fault.c
											//      asm_run_mythryl_task		def in    src/c/machine-dependent/prim.pwrpc32.asm
											//      asm_run_mythryl_task		def in    src/c/machine-dependent/prim.sparc32.asm
											// true_asm_run_mythryl_task		def in    true_asm_run_mythryl_task

	// 'request' is one of the request codes defined in
	//
	//     src/c/h/asm-to-c-request-codes.h
	//
	// The request code was passed to us from an assembly code
	// function in one of the platform-specific files
	//
        //     src/c/machine-dependent/prim.pwrpc32.asm
        //     src/c/machine-dependent/prim.intel32.asm
        //     src/c/machine-dependent/prim.sparc32.asm
        //     src/c/machine-dependent/prim.intel32.masm
	//
	// which in turn maybe have been called directly from
	// Mythryl via the runtime::asm API defined in
	//
	//     src/lib/core/init/runtime.api
	//
	// We may also arrive here because the cleaner limit
	// probe code noticed a condition which we need to handle,
	// usually either that it is time to do a garbage collection
	// or that a POSIX interprocess signal (e.g., SIGALRM) needs
	// to be handled.


	previous_profile_index =   DEREF( THIS_FN_PROFILING_HOOK_REFCELL_GLOBAL );

	ASSIGN( THIS_FN_PROFILING_HOOK_REFCELL_GLOBAL, PROF_RUNTIME );

	if (request == REQUEST_CLEANING) {
	    //
	    if (pthread->posix_signal_pending) {
		//
		// This "request" is really a POSIX interprocess signal.

		if (need_to_clean_heap( task, 4*ONE_K_BINARY )) {
		    //
		    clean_heap( task, 0 );
		}

	        // Figure out which unix signal needs handling
		// and save its (number, count) in
		//
                //    pthread->next_posix_signal_id,		// SIGALRM or whatever.
                //    pthread->next_posix_signal_count		// Number of times it has happened since last being handled.
		//
		// choose_signal() and make_mythryl_signal_handler_arg() are both from
		//
		//     src/c/machine-dependent/signal-stuff.c
		//
		// Our actual kernel-invoked signal handler is   c_signal_handler()   from
		//
		//     src/c/machine-dependent/posix-signal.c
		//
		// POSIX_INTERPROCESS_SIGNAL_HANDLER_REFCELL_GLOBAL in practice points to
		//
		//     root_mythryl_handler_for_posix_interprocess_signals ()
		//
		// from
                //
                //     src/lib/std/src/nj/runtime-signals-guts.pkg
                //
 		// resume_after_handling_signal  is assembly code from
		// (depending upon platform) one of:
		//     src/c/machine-dependent/prim.intel32.asm
		//     src/c/machine-dependent/prim.intel32.masm
                //     src/c/machine-dependent/prim.sparc32.asm
                //     src/c/machine-dependent/prim.pwrpc32.asm
		//
		// We generate
                //     return_from_signal_handler_c
                // via a
                //     ASM_CONT( return_from_signal_handler )
                // statement in
		//     src/c/main/construct-runtime-package.c
		//
		choose_signal( pthread );
		//
		task->argument	     =  make_mythryl_signal_handler_arg( task, resume_after_handling_signal );
		task->fate	     =  PTR_CAST( Val,  return_from_signal_handler_c );
		task->exception_fate =  PTR_CAST( Val, handle_uncaught_exception_closure_v+1);
		task->closure	     =  DEREF( POSIX_INTERPROCESS_SIGNAL_HANDLER_REFCELL_GLOBAL );
		//
		task->program_counter =
		task->link_register   = GET_CODE_ADDRESS_FROM_CLOSURE( task->closure );
		//
		pthread->mythryl_handler_for_posix_signal_is_running	    = TRUE;
		//
		pthread->posix_signal_pending= FALSE;
	    }
#ifdef SOFTWARE_GENERATED_PERIODIC_EVENTS
	    else if (task->software_generated_periodic_event_is_pending
                 && !task->in_software_generated_periodic_event_handler
                 ){ 
	      // This is a software-generated periodic event:
              //
#if defined(MULTICORE_SUPPORT) && defined(MULTICORE_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS)
	      // Note: under MULTICORE, software generated periodic
              // events are used only for garbage collection.
              //
    #ifdef SOFTWARE_GENERATED_PERIODIC_EVENTS_DEBUG
	debug_say ("run-mythryl-code-and-runtime-eventloop: software generated periodic event\n");
    #endif
	        task->software_generated_periodic_event_is_pending = FALSE;
	        clean_heap(
                    task,
                    0		// Age-groups to clean.  (=="Generations to garbage collect".)
                );
#else
		if (need_to_clean_heap( task, 4 * ONE_K_BINARY )) {					// 64-bit issue -- '4' here is 'bytes-per-word'.
													// This 4*ONE_K_BINARY number has(?) to match   max_heapwords_to_allocate_between_heaplimit_checks
													//     in src/lib/compiler/back/low/main/fatecode/pick-fatecode-funs-for-heaplimit-checks.pkg
													// This 4*ONE_K_BINARY number has(?) to match   skid_pad_size_in_bytes
													//     in   src/lib/compiler/back/low/main/fatecode/emit-treecode-heapcleaner-calls-g.pkg
		    clean_heap (task, 0);
                }
		task->argument	     =  make_resumption_fate(task, resume_after_handling_software_generated_periodic_event);	// make_resumption_fate is from  src/c/machine-dependent/signal-stuff.c
		task->fate	     =  PTR_CAST( Val, return_from_software_generated_periodic_event_handler_c);
		task->exception_fate =  PTR_CAST( Val, handle_v+1);
		task->closure	     =  DEREF( SOFTWARE_GENERATED_PERIODIC_EVENTS_HANDLER_REFCELL_GLOBAL );
		//
		task->program_counter=
		task->link_register  = GET_CODE_ADDRESS_FROM_CLOSURE(task->closure);
		//
		task->in_software_generated_periodic_event_handler= TRUE;
		task->software_generated_periodic_event_is_pending	= FALSE;
#endif // MULTICORE_SUPPORT
	    } 
#endif // SOFTWARE_GENERATED_PERIODIC_EVENTS
	    else
	        clean_heap (task, 0);
	} else {

	    switch (request) {
	        //
	    case REQUEST_RETURN_TO_C_LEVEL:
		// Here to return to whoever
                // called us.	    If our caller was   load_and_run_heap_image                 in   src/c/main/load-and-run-heap-image.c
		// this will return us to               main                                    in   src/c/main/runtime-main.c
		// which will print stats
		// and exit(), but  if our caller was   no_args_entry  or  some_args_entry      in   src/c/lib/ccalls/ccalls-fns.c
		// then we may have some scenario
		// where C calls Mythryl which calls C which ...
		// and we may just be unwinding on level.
		//    The latter can only happen with the
		// help of the src/lib/c-glue-old/ stuff,
		// which is currently non-operational.
		//
		// We are also called by
		//     src/c/multicore/sgi-multicore.c
		//     src/c/multicore/solaris-multicore.c
		// but that stuff is also non-operational (I think) and
		// we're not supposed to return to caller in those cases.
		// 
		clean_heap (task, 0);	        		// Do a minor collection to clear the store list.
		return;

	    case REQUEST_HANDLE_UNCAUGHT_EXCEPTION:
	        handle_uncaught_exception( task->argument );	// handle_uncaught_exception	def in    src/c/main/runtime-exception-stuff.c
		return;

	    case REQUEST_FAULT:                    			// A hardware fault.
                {
		    Val	  loc;
		    Val	  traceStk;
		    Val	  exn;
		    Unt8* namestring =  codechunk_comment_string_for_program_counter( task->faulting_program_counter );		// codechunk_comment_string_for_program_counter	def in   src/c/cleaner/hugechunk.c

		    if (namestring != NULL) {
		        //
			char	buf2[192];
			sprintf(buf2, "<file %.184s>", namestring);
			loc = make_ascii_string_from_c_string( task, buf2 );

		    } else {

			loc = make_ascii_string_from_c_string( task, "<unknown file>" );
		    }
		    LIST_CONS( task, traceStk, loc, LIST_NIL );
		    EXN_ALLOC( task, exn, task->fault_exception, HEAP_VOID, traceStk );
		    raise_mythryl_exception( task, exn );
		}
                break;

	    case REQUEST_FIND_CFUN:
		task->argument
                    =
		    find_mythryl_callable_c_function (									// find_mythryl_callable_c_function	def in    src/c/lib/mythryl-callable-c-libraries.c
		        HEAP_STRING_AS_C_STRING( GET_TUPLE_SLOT_AS_VAL( task->argument, 0) ),
		        HEAP_STRING_AS_C_STRING( GET_TUPLE_SLOT_AS_VAL( task->argument, 1) )
                    );
		SET_UP_RETURN( task );
		break;

	    case REQUEST_CALL_CFUN:
		{
		    Val    (*f)(), arg;

		    SET_UP_RETURN( task );

		    if (need_to_clean_heap (task, 8*ONE_K_BINARY)) {
			clean_heap (task, 0);
		    }
#ifdef INDIRECT_CFUNC
		    f = ((Mythryl_Name_With_C_Function*) GET_TUPLE_SLOT_AS_PTR( Val_Sized_Unt*, task->argument, 0 ))->cfunc;
#  ifdef DEBUG_TRACE_CCALL
		    debug_say("CALLC: %s (%#x)\n",
			((Mythryl_Name_With_C_Function*) GET_TUPLE_SLOT_AS_PTR( Val_Sized_Unt*, task->argument, 0 ))->name,
			GET_TUPLE_SLOT_AS_VAL(task->argument, 1));
#  endif
#else
		    f = (Mythryl_Callable_C_Function) GET_TUPLE_SLOT_AS_PTR( Val_Sized_Unt*, task->argument, 0 );
#  ifdef DEBUG_TRACE_CCALL
		    debug_say("CALLC: %#x (%#x)\n", f, GET_TUPLE_SLOT_AS_VAL(task->argument, 1));
#  endif
#endif
		    arg  = GET_TUPLE_SLOT_AS_VAL( task->argument, 1 );

		    task->argument
			=
			(*f)(task, arg);
		}
		break;

	    case REQUEST_ALLOCATE_STRING:
		task->argument =   allocate_nonempty_ascii_string( task, INT31_TO_C_INT( task->argument ) );
		SET_UP_RETURN( task );
		break;

	    case REQUEST_ALLOCATE_BYTE_VECTOR:
		task->argument =   allocate_nonempty_unt8_vector( task, INT31_TO_C_INT(task->argument) );
		SET_UP_RETURN( task );
		break;

	    case REQUEST_ALLOCATE_FLOAT64_VECTOR:
		task->argument =   allocate_nonempty_float64_vector( task, INT31_TO_C_INT(task->argument) );
		SET_UP_RETURN( task );
		break;

	    case REQUEST_MAKE_TYPEAGNOSTIC_RW_VECTOR:
		task->argument =   make_nonempty_rw_vector( task, GET_TUPLE_SLOT_AS_INT(task->argument, 0), GET_TUPLE_SLOT_AS_VAL(task->argument, 1) );
		SET_UP_RETURN( task );
		break;

	    case REQUEST_MAKE_TYPEAGNOSTIC_RO_VECTOR:
		task->argument = make_nonempty_ro_vector( task, GET_TUPLE_SLOT_AS_INT(task->argument, 0), GET_TUPLE_SLOT_AS_VAL(task->argument, 1) );
		SET_UP_RETURN( task );
		break;

	    case REQUEST_RETURN_FROM_SIGNAL_HANDLER:
#ifdef SIGNAL_DEBUG
debug_say("REQUEST_RETURN_FROM_SIGNAL_HANDLER: arg = %#x, pending = %d, inHandler = %d, nSigs = %d/%d\n",
task->argument, pthread->posix_signal_pending, pthread->mythryl_handler_for_posix_signal_is_running,
pthread->all_posix_signals.done_count, pthread->all_posix_signals.seen_count);
#endif

	        // Throw to the fate:
		//
 		SET_UP_THROW( task, task->argument, HEAP_VOID );


	        // Note that we are exiting the handler:
		//
		pthread->mythryl_handler_for_posix_signal_is_running = FALSE;
		break;


#ifdef SOFTWARE_GENERATED_PERIODIC_EVENTS

	    case REQUEST_RETURN_FROM_SOFTWARE_GENERATED_PERIODIC_EVENT_HANDLER:
		SET_UP_THROW( task, task->argument, HEAP_VOID );	        // Throw to the fate.
		//
		task->in_software_generated_periodic_event_handler	        // Note that we are exiting the handler.
                    =
                    FALSE;
		//
		reset_heap_allocation_limit_for_software_generated_periodic_events( task );
		break;
#endif

#ifdef SOFTWARE_GENERATED_PERIODIC_EVENTS
	    case REQUEST_RESUME_SOFTWARE_GENERATED_PERIODIC_EVENT_HANDLER:
#endif
	    case REQUEST_RESUME_SIGNAL_HANDLER:
		#ifdef SIGNAL_DEBUG
		    debug_say("REQUEST_RESUME_SIGNAL_HANDLER: arg = %#x\n", task->argument);
		#endif
		    load_resume_state( task );					// load_resume_state	def in    src/c/machine-dependent/signal-stuff.c
		break;

	    case REQUEST_MAKE_PACKAGE_LITERALS_VIA_BYTECODE_INTERPRETER:
		die ("MAKE_PACKAGE_LITERALS_VIA_BYTECODE_INTERPRETER request");
		break;

	    default:
		die ("unknown request code = %d", request);
		break;
	    }
	}
    }									// while
}									// fun run_mythryl_task_and_runtime_eventloop



// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

