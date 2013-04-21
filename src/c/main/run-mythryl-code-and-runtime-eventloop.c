// run-mythryl-code-and-runtime-eventloop.c

#include "../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "runtime-base.h"
#include "runtime-configuration.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "asm-to-c-request-codes.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-globals.h"
#include "system-dependent-signal-stuff.h"
#include "mythryl-callable-c-libraries.h"
#include "profiler-call-counts.h"
#include "heapcleaner.h"

// Set up the return linkage and fate
// throwing in the Mythryl state vector.
//
#define SET_UP_RETURN( _task )	{							\
	    Task* __task    = (_task);							\
	    __task->current_closure = HEAP_VOID;					\
	    __task->program_counter = __task->fate;					\
	}

#define SET_UP_THROW( _task, _fate, _val )	{					\
	    Task* __task	 = (_task);						\
	    Val	__fate	         = (_fate);						\
	    __task->current_closure  = __fate;						\
	    __task->fate         = HEAP_VOID;						\
	    __task->program_counter=							\
	    __task->link_register  = GET_CODE_ADDRESS_FROM_CLOSURE( __fate );		\
	    __task->exception_fate = HEAP_VOID;						\
	    __task->argument	 = (_val);						\
	}



Val   run_mythryl_function__may_heapclean   (Task* task,  Val function,  Val argument,  Bool use_fate,  Roots* extra_roots)   {
    //===================================
    //
    // Apply the Mythryl closure 'function' to 'argument' and return the result.
    // If the flag   use_fate   is set, then the Task has already
    // been initialized with a return fate by save_c_state.

    initialize_task( task );								// initialize_task				def in   src/c/main/runtime-state.c

    // Initialize the calling context:
    //
    task->exception_fate =  PTR_CAST( Val,  handle_uncaught_exception_closure_v + 1 );
    task->current_thread     =  HEAP_VOID;
    task->argument	 =  argument;

    if (!use_fate)     task->fate = PTR_CAST( Val,  return_to_c_level_c );		// See   ASM_CONT(return_to_c_level);   in   src/c/main/construct-runtime-package.c

    task->current_closure   = function;

    task->program_counter  =
    task->link_register	   = GET_CODE_ADDRESS_FROM_CLOSURE( function );

    run_mythryl_task_and_runtime_eventloop__may_heapclean( task, extra_roots );		// run_mythryl_task_and_runtime_eventloop__may_heapclean	def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c

    return task->argument;
}


extern int  asm_run_mythryl_task  (Task* task);						// asm_run_mythryl_task	def in   src/c/machine-dependent/prim.intel32.asm
											// asm_run_mythryl_task	def in   src/c/machine-dependent/prim.pwrpc32.asm
											// asm_run_mythryl_task	def in   src/c/machine-dependent/prim.sparc32.asm
											//
											// asm_run_mythryl_task	def in   src/c/machine-dependent/win32-fault.c

// run_mythryl_task_and_runtime_eventloop__may_heapclean
//
#if !defined(__CYGWIN32__)

void   run_mythryl_task_and_runtime_eventloop__may_heapclean   (Task* task, Roots* extra_roots)   {
    // =====================================================
    //
    // Called from:
    //     src/c/main/load-and-run-heap-image.c
    //     src/c/lib/ccalls/ccalls-fns.c

#else

  void   system_run_mythryl_task_and_runtime_eventloop__may_heapclean   (Task *task, Roots* extra_roots)   {		// called from			 src/c/machine-dependent/cygwin-fault.c
//       ============================================================

#endif

    int		request;

    Hostthread* hostthread  =  task->hostthread;

    Val	previous_profile_index =  IN_OTHER_CODE__CPU_USER_INDEX;				// IN_OTHER_CODE__CPU_USER_INDEX	is from   src/c/h/profiler-call-counts.h
												// previous_profile_index is a tagged int, so it is safe from heapcleaning.
    for (;;) {
// ramlog_printf("#%d runtime_eventloop/TOP\n", syscalls_seen );

        //     THIS_FN_PROFILING_HOOK_REFCELL__GLOBAL
        // is #defined in
        //     src/c/h/runtime-globals.h
        // in terms of
        //     this_fn_profiling_hook_refcell__global
        // from
        //     src/c/main/construct-runtime-package.c


	//     IN_RUNTIME__CPU_USER_INDEX
	// is #defined in
	//     src/c/h/profiler-call-counts.h

	ASSIGN( THIS_FN_PROFILING_HOOK_REFCELL__GLOBAL, previous_profile_index );

	request = asm_run_mythryl_task( task );						//      asm_run_mythryl_task		def in    src/c/machine-dependent/prim.intel32.asm
											//      asm_run_mythryl_task		def in    src/c/machine-dependent/win32-fault.c
											//      asm_run_mythryl_task		def in    src/c/machine-dependent/prim.pwrpc32.asm
											//      asm_run_mythryl_task		def in    src/c/machine-dependent/prim.sparc32.asm
											// true_asm_run_mythryl_task		def in    true_asm_run_mythryl_task

// ramlog_printf("#%d runtime_eventloop back from asm_run_mythryl_task\n", syscalls_seen );
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


	previous_profile_index =   DEREF( THIS_FN_PROFILING_HOOK_REFCELL__GLOBAL );

	ASSIGN( THIS_FN_PROFILING_HOOK_REFCELL__GLOBAL, IN_RUNTIME__CPU_USER_INDEX );			// Remember that from here CPU cycles are charged to the runtime.

	if (request == REQUEST_HEAPCLEANING) {
ramlog_printf("#%d runtime_eventloop:  request == REQUEST_HEAPCLEANING\n", syscalls_seen );
	    //
	    if (hostthread->interprocess_signal_pending) {						// interprocess_signal_pending		gets set by   c_signal_handler()	from   src/c/machine-dependent/interprocess-signals.c
		//
		// This "request" is really a POSIX interprocess signal.

ramlog_printf("#%d runtime_eventloop:  request == REQUEST_HEAPCLEANING: hostthread->interprocess_signal_pending = TRUE, ->id d=%d ->name s='%s'\n", syscalls_seen,hostthread->id,hostthread->name);
		if (need_to_call_heapcleaner( task, 4*ONE_K_BINARY )) {
		    //
ramlog_printf("#%d runtime_eventloop:  request == REQUEST_HEAPCLEANING: hostthread->interprocess_signal_pending = TRUE, CALLING HEAPCLEANER  ->id d=%d ->name s='%s'\n", syscalls_seen,hostthread->id,hostthread->name);
		    call_heapcleaner( task, 0 );							// call_heapcleaner	def in   src/c/heapcleaner/call-heapcleaner.c
ramlog_printf("#%d runtime_eventloop:  request == REQUEST_HEAPCLEANING: hostthread->interprocess_signal_pending = TRUE, CALLED  HEAPCLEANER  ->id d=%d ->name s='%s'\n", syscalls_seen,hostthread->id,hostthread->name);
		}

	        // Figure out which interprocess signal needs handling
		// and save its (number, count) in
		//
                //    hostthread->next_posix_signal_id,			// SIGALRM or whatever.
                //    hostthread->next_posix_signal_count		// Number of times it has happened since last being handled.
		//
		// choose_signal() 			is from   src/c/machine-dependent/interprocess-signals.c
		// make_mythryl_signal_handler_arg()	is from   src/c/machine-dependent/signal-stuff.c
		//
		// Our actual kernel-invoked signal handler is   c_signal_handler()   from
		//
		//     src/c/machine-dependent/interprocess-signals.c
		//
		// POSIX_INTERPROCESS_SIGNAL_HANDLER_REFCELL__GLOBAL in practice points to
		//
		//     root_mythryl_handler_for_interprocess_signals ()
		//
		// from
                //
                //     src/lib/std/src/nj/interprocess-signals-guts.pkg
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
		choose_signal( hostthread );								// choose_signal	is from   src/c/machine-dependent/interprocess-signals.c
		//
		task->argument	      =  make_mythryl_signal_handler_arg( task, resume_after_handling_signal );
		task->fate	      =  PTR_CAST( Val,  return_from_signal_handler_c );
		task->exception_fate  =  PTR_CAST( Val,  handle_uncaught_exception_closure_v + 1 );
		task->current_closure =  DEREF( POSIX_INTERPROCESS_SIGNAL_HANDLER_REFCELL__GLOBAL );
		//
		task->program_counter =
		task->link_register   = GET_CODE_ADDRESS_FROM_CLOSURE( task->current_closure );
		//
ramlog_printf("#%d runtime_eventloop:  setting hostthread->mythryl_handler_for_interprocess_signal_is_running =  TRUE\n", syscalls_seen );
		hostthread->mythryl_handler_for_interprocess_signal_is_running =  TRUE;
		//
		hostthread->interprocess_signal_pending= FALSE;
	    }
#if NEED_SOFTWARE_GENERATED_PERIODIC_EVENTS
	    else if (task->software_generated_periodic_event_is_pending
                 && !task->in_software_generated_periodic_event_handler
                 ){ 
	      // This is a software-generated periodic event:
              //
#if NEED_HOSTTHREAD_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	      //
	      // Note: Under current hostthread support design,
	      //       software generated periodic
              //       events are used only for garbage collection.
              //
    #ifdef SOFTWARE_GENERATED_PERIODIC_EVENTS_DEBUG
	debug_say ("run-mythryl-code-and-runtime-eventloop: software generated periodic event\n");
    #endif
	        task->software_generated_periodic_event_is_pending = FALSE;
	        call_heapcleaner(
                    task,
                    0		// Age-groups to heapclean.  (=="Generations to garbage collect".)
                );
#else
		if (need_to_call_heapcleaner( task, 4 * ONE_K_BINARY )) {				// 64-bit issue -- '4' here is 'bytes-per-word'.
													// This 4*ONE_K_BINARY number has(?) to match   max_heapwords_to_allocate_between_heaplimit_checks
													//     in src/lib/compiler/back/low/main/nextcode/pick-nextcode-fns-for-heaplimit-checks.pkg
													// This 4*ONE_K_BINARY number has(?) to match   skid_pad_size_in_bytes
													//     in   src/lib/compiler/back/low/main/nextcode/emit-treecode-heapcleaner-calls-g.pkg
		    call_heapcleaner (task, 0);
                }
		task->argument	      =  make_posthandler_resumption_fate_from_task(task, resume_after_handling_software_generated_periodic_event);	// make_posthandler_resumption_fate_from_task is from  src/c/machine-dependent/signal-stuff.c
		task->fate	      =  PTR_CAST( Val, return_from_software_generated_periodic_event_handler_c);
		task->exception_fate  =  PTR_CAST( Val, handle_uncaught_exception_closure_v + 1 );
		task->current_closure =  DEREF( SOFTWARE_GENERATED_PERIODIC_EVENTS_HANDLER_REFCELL__GLOBAL );
		//
		task->program_counter =
		task->link_register   =  GET_CODE_ADDRESS_FROM_CLOSURE( task->current_closure );
		//
		task->in_software_generated_periodic_event_handler =  TRUE;
		task->software_generated_periodic_event_is_pending =  FALSE;

#endif // NEED_HOSTTHREAD_SUPPORT_FOR_SOFTWARE_GENERATED_PERIODIC_EVENTS
	    } 
#endif // NEED_SOFTWARE_GENERATED_PERIODIC_EVENTS
	    else
	        call_heapcleaner (task, 0);
	} else {

	    switch (request) {
	        //
	    case REQUEST_RETURN_TO_C_LEVEL:
		// Here to return to whoever
                // called us.	    If our caller was   load_and_run_heap_image__may_heapclean  in   src/c/main/load-and-run-heap-image.c
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
		//     src/c/hostthread/hostthread-on-posix-threads.c
		//     src/c/hostthread/hostthread-on-sgi.c
		//     src/c/hostthread/hostthread-on-solaris.c
		// but that stuff is also non-operational (I think) and
		// we're not supposed to return to caller in those cases.
		// 
		call_heapcleaner (task, 0);	        		// Do a minor collection to clear the store list.
		return;

	    case REQUEST_HANDLE_UNCAUGHT_EXCEPTION:
	        handle_uncaught_exception( task->argument );		// handle_uncaught_exception	def in    src/c/main/runtime-exception-stuff.c
		return;

	    case REQUEST_FAULT:                    			// A hardware fault.
                {
		    Unt8* namestring =  codechunk_comment_string_for_program_counter( task->faulting_program_counter );		// codechunk_comment_string_for_program_counter	def in   src/c/heapcleaner/hugechunk.c
		    //
		    Val loc;
		    if (namestring != NULL) {
		        //
			char	buf2[192];
			sprintf(buf2, "<file %.184s>", namestring);
			loc = make_ascii_string_from_c_string__may_heapclean( task, buf2, extra_roots );

		    } else {

			loc = make_ascii_string_from_c_string__may_heapclean( task, "<unknown file>", extra_roots );
		    }

		    Val trace_stack =  LIST_CONS( task, loc, LIST_NIL );

		    Val exception   =  MAKE_EXCEPTION( task, task->fault_exception, HEAP_VOID, trace_stack );

		    raise_mythryl_exception( task, exception );
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
		    Val    (*f)();
		    Val    arg;

		    SET_UP_RETURN( task );

		    if (need_to_call_heapcleaner (task, 8*ONE_K_BINARY)) {
		        //
			call_heapcleaner (task, 0);
		    }
#ifdef INDIRECT_CFUNC
		    f = ((Mythryl_Name_With_C_Function*) GET_TUPLE_SLOT_AS_PTR( Vunt*, task->argument, 0 ))->cfunc;
#  ifdef DEBUG_TRACE_CCALL
		    debug_say("CALLC: %s (%#x)\n",
			((Mythryl_Name_With_C_Function*) GET_TUPLE_SLOT_AS_PTR( Vunt*, task->argument, 0 ))->name,
			GET_TUPLE_SLOT_AS_VAL(task->argument, 1));
#  endif
#else
		    f = (Mythryl_Callable_C_Function) GET_TUPLE_SLOT_AS_PTR( Vunt*, task->argument, 0 );
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
		task->argument =   allocate_nonempty_ascii_string__may_heapclean( task, TAGGED_INT_TO_C_INT( task->argument ), extra_roots );
		SET_UP_RETURN( task );
		break;

	    case REQUEST_ALLOCATE_BYTE_VECTOR:
		task->argument =   allocate_nonempty_vector_of_one_byte_unts__may_heapclean( task, TAGGED_INT_TO_C_INT(task->argument), extra_roots );
		SET_UP_RETURN( task );
		break;

	    case REQUEST_ALLOCATE_VECTOR_OF_EIGHT_BYTE_FLOATS:
		task->argument =   allocate_nonempty_vector_of_eight_byte_floats__may_heapclean( task, TAGGED_INT_TO_C_INT(task->argument), extra_roots );
		SET_UP_RETURN( task );
		break;

	    case REQUEST_MAKE_TYPEAGNOSTIC_RW_VECTOR:
		task->argument =   make_nonempty_rw_vector__may_heapclean( task, GET_TUPLE_SLOT_AS_INT(task->argument, 0), GET_TUPLE_SLOT_AS_VAL(task->argument, 1), extra_roots );
		SET_UP_RETURN( task );
		break;

	    case REQUEST_MAKE_TYPEAGNOSTIC_RO_VECTOR:
		task->argument =   make_nonempty_ro_vector__may_heapclean( task, GET_TUPLE_SLOT_AS_INT(task->argument, 0), GET_TUPLE_SLOT_AS_VAL(task->argument, 1), extra_roots );
		SET_UP_RETURN( task );
		break;

	    case REQUEST_RETURN_FROM_SIGNAL_HANDLER:
#ifdef SIGNAL_DEBUG
debug_say("REQUEST_RETURN_FROM_SIGNAL_HANDLER: arg = %#x, pending = %d, inHandler = %d, nSigs = %d/%d\n",
task->argument, hostthread->interprocess_signal_pending, hostthread->mythryl_handler_for_interprocess_signal_is_running,
hostthread->all_posix_signals.done_count, hostthread->all_posix_signals.seen_count);
#endif

	        // Throw to the fate:
		//
 		SET_UP_THROW( task, task->argument, HEAP_VOID );


	        // Note that we are exiting the handler:
		//
ramlog_printf("#%d runtime_eventloop/REQUEST_RETURN_FROM_SIGNAL_HANDLER:  setting hostthread->mythryl_handler_for_interprocess_signal_is_running =  FALSE\n", syscalls_seen );
		hostthread->mythryl_handler_for_interprocess_signal_is_running =  FALSE;
		break;


#if NEED_SOFTWARE_GENERATED_PERIODIC_EVENTS

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

#if NEED_SOFTWARE_GENERATED_PERIODIC_EVENTS
	    case REQUEST_RESUME_AFTER_RUNNING_SOFTWARE_GENERATED_PERIODIC_EVENT_HANDLER:
#endif
	    case REQUEST_RESUME_AFTER_RUNNING_SIGNAL_HANDLER:
		#ifdef SIGNAL_DEBUG
		    debug_say("REQUEST_RESUME_AFTER_RUNNING_SIGNAL_HANDLER: arg = %#x\n", task->argument);
		#endif
		    load_task_from_posthandler_resumption_fate( task );			// load_task_from_posthandler_resumption_fate	def in    src/c/machine-dependent/signal-stuff.c
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
}									// fun run_mythryl_task_and_runtime_eventloop__may_heapclean



// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

