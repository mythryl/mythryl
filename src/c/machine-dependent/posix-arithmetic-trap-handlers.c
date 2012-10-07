// posix-arithmetic-trap-handlers.c
//
// Common code for handling arithmetic traps.

#include <stdio.h>
#include <sys/types.h>		// For getpid().
#include <unistd.h>		// For getpid(), sleep(). 

#if defined(__CYGWIN32__)

#include "../mythryl-config.h"

#include "cygwin-fault.c"

#else

#include "../mythryl-config.h"

#include "system-dependent-unix-stuff.h"
#include "system-dependent-signal-get-set-etc.h"
#include "runtime-base.h"
#include "runtime-globals.h"
#include "heap.h"

// This is temporary:					XXX BUGGO FIXME
//
#define SELF_HOSTTHREAD	(hostthread_table__global[ 0 ])			// Note that we have switched to  #define SELF_HOSTTHREAD	(pth__get_hostthread())    in   src/c/machine-dependent/posix-signal.c


static void   arithmetic_fault_handler   (/* int sig, Signal_Handler_Info_Arg code, Signal_Handler_Context_Arg* scp */);



void   set_up_fault_handlers   (Task* task)   {
    // =====================
    //
    // Set up the DIVIDE_BY_ZERO and OVERFLOW fault handlers

    #ifdef SIG_FAULT1
	SET_SIGNAL_HANDLER( SIG_FAULT1, arithmetic_fault_handler );
    #endif
    #ifdef SIG_FAULT2
	SET_SIGNAL_HANDLER( SIG_FAULT2, arithmetic_fault_handler );
    #endif

    SET_UP_FLOATING_POINT_EXCEPTION_HANDLING ();				// Initialize the floating-point unit.
}

static void   enter_debug_loop   (void) {
    //        ================
    //
    syscall_log_and_ramlog_enabled = FALSE;					// To keep from muddying the evidence, disable further changes to the syscall_log and ramlog.
    fprintf(stderr, "Done:  syscall_log_and_ramlog_enabled = FALSE;\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "To kill this process do ^Z to suspend it and then:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "%% kill -HUP %d\n", getpid());
    fprintf(stderr, "\n");
    fprintf(stderr, "and then 'fg' to let it complete exit.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "To attach gdb to this process\n");
    fprintf(stderr, "do ^Z to suspend it and then:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "%% gdb mythryl-runtime-intel32 %d\n", getpid());
    fprintf(stderr, "\n");
    fprintf(stderr, "Useful commands to try after attaching include:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "(gdb) bt                   # To display callstack.\n");
    fprintf(stderr, "(gdb) call debug_help()    # To list debug-support fns callable from gdb prompt.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Now doing:    while(TRUE) sleep(5);\n");
    fflush(stderr);
    while (TRUE)  sleep(5);

//	    fprintf(stderr, "error: Dumping heap due to uncaught signal while running in Mythryl C layer: signal = %d, signal code = %#x, pc = %#x. (Check logfile for details). -- %d:%s\n", signal, GET_SIGNAL_CODE(si, scp), GET_SIGNAL_PROGRAM_COUNTER(scp), __LINE__, __FILE__);fflush(stderr);
//	    dump_all( SELF_HOSTTHREAD->task, "arithmetic_fault_handler" );				// dump_all	is from   src/c/heapcleaner/heap-debug-stuff.c
//  	    //
//            fprintf(stderr,"\n=================================================================================\n"); fflush(stderr);
//	    die ("Exiting due to uncaught signal while running in Mythryl C layer: signal = %d, signal code = %#x, pc = %#x    -- %d:%s\n", signal, GET_SIGNAL_CODE(si, scp), GET_SIGNAL_PROGRAM_COUNTER(scp), __LINE__, __FILE__);
}


// arithmetic_fault_handler:
//
// Handle arithmetic faults (e.g., divide by zero, integer overflow).
//
#if defined(HAS_POSIX_SIGS) && defined(HAS_UCONTEXT)

    static void   arithmetic_fault_handler   (int signal,  siginfo_t* si,  void* c)   {
        //        ========================
	//
	ucontext_t* scp = (ucontext_t*) c;

	Task*  task =   SELF_HOSTTHREAD->task;
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

	extern Vunt   request_fault[]; 

	int code =  GET_SIGNAL_CODE( si, scp );

	#ifdef SIGNAL_DEBUG
	    debug_say ("Fault handler: signal = %d, inLib7 = %d\n", signal, SELF_HOSTTHREAD->executing_mythryl_code);
	#endif

	if (! SELF_HOSTTHREAD->executing_mythryl_code) {
  	    //
	    fprintf(stderr,"\n=================================================================================\n");
	    fprintf(stderr, "error: Uncaught signal while running in Mythryl C layer: signal = %d, signal code = %#x, pc = %#x. -- %d:%s\n", signal, GET_SIGNAL_CODE(si, scp), GET_SIGNAL_PROGRAM_COUNTER(scp), __LINE__, __FILE__);
	    fflush( stderr);
	    //
	    enter_debug_loop();
	}

	// Map the signal to the appropriate Mythryl exception:
        //
	if (INT_OVFLW(signal, code)) {								// INT_OVFLW	is from   src/c/h/system-dependent-signal-get-set-etc.h 
	    //
	    task->fault_exception = OVERFLOW_EXCEPTION__GLOBAL;					// OVERFLOW_EXCEPTION__GLOBAL	is from   src/c/h/runtime-globals.h
	    task->faulting_program_counter = (Vunt)GET_SIGNAL_PROGRAM_COUNTER(scp);
	    //
	} else if (INT_DIVZERO(signal, code)) {
	    //
	    task->fault_exception = DIVIDE_EXCEPTION__GLOBAL;
	    task->faulting_program_counter = (Vunt)GET_SIGNAL_PROGRAM_COUNTER(scp);

	} else {
	    //
	    die ("unexpected fault, signal = %d, signal code = %#x", signal, code);
	}

	SET_SIGNAL_PROGRAM_COUNTER( scp, request_fault );

	RESET_FLOATING_POINT_EXCEPTION_HANDLING( scp );
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    }

#else

    static void  arithmetic_fault_handler  (
	//       ========================
	//
	int		    signal,
	#if (defined(TARGET_PWRPC32) && defined(OPSYS_LINUX))
	    Signal_Handler_Context_Arg*  scp						// "scp" may be "signalhandler context pointer"
	#else
	    Signal_Handler_Info_Arg	  info,
	    Signal_Handler_Context_Arg*   scp
	#endif
    ){
	Task* task =  SELF_HOSTTHREAD->task;
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

	extern Vunt   request_fault[]; 

	int code =  GET_SIGNAL_CODE( info, scp );

	#ifdef SIGNAL_DEBUG
	    debug_say ("Fault handler: signal = %d, inLib7 = %d\n", signal, SELF_HOSTTHREAD->executing_mythryl_code);
	#endif

        if (! SELF_HOSTTHREAD->executing_mythryl_code) {
	    //
	    fprintf(stderr,"\n=================================================================================\n");
	    fprintf(stderr, "error: Uncaught signal while running in Mythryl C layer: signal = %d, signal code = %#x, pc = %#x. -- %d:%s\n", signal, GET_SIGNAL_CODE(si, scp), GET_SIGNAL_PROGRAM_COUNTER(scp), __LINE__, __FILE__);
	    fflush( stderr);
	    //
	    enter_debug_loop();
	}

        // Map the signal to the appropriate Mythryl exception:
        //
	if (INT_OVFLW(signal, code)) {
	    //
	    task->fault_exception = OVERFLOW_EXCEPTION__GLOBAL;
	    task->faulting_program_counter = (Vunt)GET_SIGNAL_PROGRAM_COUNTER( scp );
	    //
	} else if (INT_DIVZERO(signal, code)) {
	    //
	    task->fault_exception = DIVIDE_EXCEPTION__GLOBAL;
	    task->faulting_program_counter = (Vunt)GET_SIGNAL_PROGRAM_COUNTER(scp);
	    //
	} else {
	    die ("unexpected fault, signal = %d, code = %#x", signal, code);
	}

	SET_SIGNAL_PROGRAM_COUNTER( scp, request_fault );

	RESET_FLOATING_POINT_EXCEPTION_HANDLING( scp );
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    }

#endif

#endif // defined(__CYGWIN32__)


// COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


