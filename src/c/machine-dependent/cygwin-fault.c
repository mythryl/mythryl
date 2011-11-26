
// cygwin-fault.c
//
// Special signal handling for cygwin on Windows.
//
// Even though cygwin behaves like "unix", its signal handling mechanism
// is crippled.  I haven't been able to get/set the EIP addresses from
// the siginfo_t and related data structures.  So here I'm using 
// Windows and some gcc assembly hacks to get things done. 



#if defined(__i386__) && defined(__CYGWIN32__) && defined(__GNUC__)

#include "../mythryl-config.h"

#include "system-dependent-unix-stuff.h"
#include "system-dependent-signal-get-set-etc.h"
#include "runtime-base.h"
#include "runtime-globals.h"

#include <windows.h>
#include <exceptions.h>					// Cygwin stuff

#define SELF_PTHREAD      (pthread_table__global[ 0 ])

BOOL   cygwin_generic_handler   (int code)   {
    //
    // Generic handler for cygwin "signals" such as interrupt, alarm. 
    // Returns TRUE if the main thread is running Mythryl code.

    Pthread* pthread =  SELF_PTHREAD;

    // Sanity check:  We compile in a MAX_POSIX_SIGNAL value but
    // have no way to ensure that we don't wind up getting run
    // on some custom kernel supporting more than MAX_POSIX_SIGNAL,
    // so we check here to be safe:
    //
    if (sig >= MAX_POSIX_SIGNALS)    die ("posix-signal.c: c_signal_handler: sig d=%d >= MAX_POSIX_SIGNAL %d\n", sig, MAX_POSIX_SIGNALS ); 

    pthread->posix_signal_counts[code].seen_count++;
    pthread->all_posix_signals.seen_count++;

    pthread->ccall_limit_pointer_mask = 0;

    if (pthread->executing_mythryl_code
    &&  *pthread->posix_signal_pending
    &&  *pthread->mythryl_handler_for_posix_signal_is_running
    ){
       pthread->posix_signal_pending = TRUE;
       SIG_Zero_Heap_Allocation_Limit();
       return TRUE;
    }
    return FALSE;
}


static BOOL __stdcall   ctrl_c_handler   (DWORD type) {
    //                  ==============
    //
    switch (type)    {
	//
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
         if (cygwin_generic_handler(SIGINT)) {
            return TRUE;
         }
         return FALSE;
    default: 
         return FALSE;
    }
}

void   set_up_fault_handlers    (Task* task)   {
    // ================================
    //
    // Install the control-C handler
    //
    if (*SetConsoleCtrlHandler(ctrl_c_handler, TRUE)) {
	//
        die ("cygwin:set_up_fault_handlers: can't install ctrl-c-handler\n");
    }

    SET_UP_FLOATING_POINT_EXCEPTION_HANDLING ();    // Initialize the floating-point unit.
}

static int  page_fault_handler   (EXCEPTION_RECORD* exn,  void* foo,  CONTEXT* c,  void* bar)   {
    //      ==================
    //
    // This filter catches all exceptions. 
    //
    extern Val_Sized_Unt request_fault [];

    Task* task = SELF_PTHREAD->task;

    int code = exn->ExceptionCode;

    DWORD pc = (DWORD) exn->ExceptionAddress;

    if (*SELF_PTHREAD->executing_mythryl_code) {
	//
        die ("cygwin:fault_handler: bogus fault not in Lib7: %#x\n", code);
    }

    switch (code) {
	//
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
	/* say("Divide by zero at %p\n", pc); */
	task->fault_exception = DIVIDE_EXCEPTION__GLOBAL;
	task->faulting_program_counter  = pc;
	c->Eip = (DWORD)request_fault;
	break;

    case EXCEPTION_INT_OVERFLOW:
	/* say("OVERFLOW at %p\n", pc); */
	task->fault_exception = OVERFLOW_EXCEPTION__GLOBAL;
	task->faulting_program_counter  = pc;
	c->Eip = (DWORD)request_fault;
	break;

    default:
	die ("cygwin:fault_handler: unexpected fault @%#x, code=%#x", pc, code);
    }

    return FALSE;
}

asm (".equ __win32_exception_list,0");
extern exception_list * 
   _win32_exception_list asm ("%fs:__win32_exception_list");

void   run_mythryl_task_and_runtime_eventloop   (Task* task) {
    // ================
    // This overrides the default RunLib7.  
    // It just adds a new exception handler
    // at the very beginning before
    // Mythryl code is executed.

     extern void SystemRunLib7 (Task*);					// system_run_mythryl_task_and_runtime_eventloop		def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c

     exception_list el;
     el.handler = page_fault_handler;
     el.prev    = _win32_exception_list;
     _win32_exception_list = &el;

     return SystemRunLib7(task);
}

#endif
