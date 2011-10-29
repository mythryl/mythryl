// win32-fault.c
//
// win32 code for handling traps (arithmetic overflow, div-by-0, ctrl-c, etc.).

#include "../mythryl-config.h"

#include <windows.h>
#include <excpt.h>

#include "runtime-base.h"
#include "pthread-state.h"
#include "task.h"
#include "runtime-globals.h"
#include "system-dependent-signal-get-set-etc.h"
#include "system-signals.h"

#include "win32-fault.h"

#define SELF_PTHREAD (pthread_table_global[ 0 ])

// Globals:
//
HANDLE win32_stdin_handle;
HANDLE win32_console_handle;
HANDLE win32_stdout_handle;
HANDLE win32_stderr_handle;

HANDLE win32_LIB7_thread_handle;
BOOL win32_isNT;

// Static globals:
//
static BOOL caught_cntrl_c = FALSE;

void   wait_for_cntrl_c() {
    //
    // We know a cntrl_c is coming; wait for it:
    //
    while (!caught_cntrl_c);
}

// Generic handler for win32 "signals" such as interrupt, alarm.
// Returns TRUE if the main thread is running Mythryl code
//
BOOL   win32_generic_handler   (int code) {

    Pthread* pthread = SELF_PTHREAD;

    pthread->posix_signal_counts[code].seen_count++;
    pthread->all_posix_signals.seen_count++;

    pthread->ccall_limit_pointer_mask = 0;

    if (pthread->executing_mythryl_code && 
      (*pthread->posix_signal_pending) && 
      (*pthread->mythryl_handler_for_posix_signal_is_running))
    {
	pthread->posix_signal_pending = TRUE;
	SIG_Zero_Heap_Allocation_Limit();
	return TRUE;
    }
    return FALSE;
}

static BOOL   cntrl_c_handler   (DWORD fdwCtrlType)    {
    //
    // The win32 handler for ctrl-c.
    //
    int ret = FALSE;

    // debug_say("event is %x\n", fdwCtrlType);

    switch (fdwCtrlType) {
    case CTRL_BREAK_EVENT:
    case CTRL_C_EVENT: {
	if (!win32_generic_handler(SIGINT)) {
	  caught_cntrl_c = TRUE;
	}
	ret = TRUE;			// We handled the event.
	break;
      }
    }
    return ret;				// Chain to other handlers.
}


void   set_up_fault_handlers   (Task* task)   {
    // =====================
    //
    // Some basic win32 initialization is done here.

    // Determine if we're NT or 95:
    //
    win32_isNT = !(GetVersion() & 0x80000000);

    // Get the redirected handle; this is "stdin":
    //
    win32_stdin_handle = GetStdHandle(STD_INPUT_HANDLE);

    // Get the actual handle of the console:
    //
    win32_console_handle = CreateFile("CONIN$",
				      GENERIC_READ|GENERIC_WRITE,
				      FILE_SHARE_READ|FILE_SHARE_WRITE,
				      NULL,
				      OPEN_EXISTING,
				      0,0);
  #ifdef WIN32_DEBUG
    if (win32_console_handle == INVALID_HANDLE_VALUE) {
      debug_say("win32: failed to get actual console handle");
    }
  #endif

    win32_stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    win32_stderr_handle = GetStdHandle(STD_ERROR_HANDLE);

  #ifdef WIN32_DEBUG
    debug_say("console input handle, %x\n", (unsigned int) win32_stdin_handle);
    debug_say("console output handle, %x\n", (unsigned int) win32_stdout_handle);
    debug_say("console error handle, %x\n", (unsigned int) win32_stderr_handle);
  #endif

    // Create a thread id for the main thread:
    //
    {
	HANDLE cp_h = GetCurrentProcess();

	if (!DuplicateHandle(cp_h,              		// process with handle to dup.
			     GetCurrentThread(),		// pseudohandle, hence the dup.
			     cp_h,        		        // handle goes to current proc.
			     &win32_LIB7_thread_handle, 	// recipient
			     THREAD_ALL_ACCESS,
			     FALSE,
			     0					// no options
			     )) {
	  die ("win32:set_up_fault_handlers: cannot duplicate thread handle");
	}
    }

    // Install the ctrl-C handler:
    //
    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)cntrl_c_handler,TRUE)) {
        //
        die("win32:set_up_fault_handlers: can't install cntrl_c_handler\n");
    }

    SET_UP_FLOATING_POINT_EXCEPTION_HANDLING ();		    // Initialize the floating-point unit.
}


static Bool   fault_handler   (int code, Val_Sized_Unt pc)   {
    //
    extern Val_Sized_Unt request_fault[];

    Task*  task =  SELF_PTHREAD->task;

    if (*SELF_PTHREAD->executing_mythryl_code) {
        die ("win32:fault_handler: bogus fault not in Lib7: %#x\n", code);
    }

    // Map the signal to the appropriate Lib7 exception:
    //
    switch (code) {
        //
    case EXCEPTION_INT_DIVIDE_BY_ZERO: 
	task->fault_exception = DIVIDE_EXCEPTION_GLOBAL;
	task->faulting_program_counter = pc;
	break;

    case EXCEPTION_INT_OVERFLOW:
	task->fault_exception = OVERFLOW_EXCEPTION_GLOBAL;
	task->faulting_program_counter = pc;
	break;

    default:
	die ("win32:fault_handler: unexpected fault @%#x, code = %#x", pc, code);
    }
    return TRUE;
}


int   asm_run_mythryl_task   (Task* task)   {
    //=================
    //
    // This is where win32 handles traps.
    // On most platforms this fn is assembly code;
    // here it is a C wrapper to the actual assembly code.

    extern Val_Sized_Unt request_fault[];

    caught_cntrl_c = FALSE;

  __try{
    int request;

    request = true_asm_run_mythryl_task( task );					// true_asm_run_mythryl_task	def in   src/c/machine-dependent/prim.intel32.masm
    return request;

  } __except(fault_handler(GetExceptionCode(), (Val_Sized_Unt *)(GetExceptionInformation())->ContextRecord->Eip) ?
#ifdef HOST_INTEL32
	     ((Val_Sized_Unt *)(GetExceptionInformation())->ContextRecord->Eip = request_fault,
              EXCEPTION_CONTINUE_EXECUTION) :
	      EXCEPTION_CONTINUE_SEARCH)
#else
#  error  non-intel32 win32 platforms need asm_run_mythryl_task support
#endif
  { /* nothing */ }
}

// COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


