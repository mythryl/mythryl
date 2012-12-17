
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

#define SELF_HOSTTHREAD	(pth__get_hostthread_by_ptid( pth__get_hostthread_ptid() ))

BOOL   cygwin_generic_handler   (int code)   {
    //
    // Generic handler for cygwin "signals" such as interrupt, alarm. 
    // Returns TRUE if the main thread is running Mythryl code.

    Hostthread* hostthread =  SELF_HOSTTHREAD;

    // Sanity check:  We compile in a SIGNAL_TABLE_SIZE_IN_SLOTS value but
    // have no way to ensure that we don't wind up getting run
    // on some custom kernel supporting more than SIGNAL_TABLE_SIZE_IN_SLOTS,
    // so we check here to be safe:
    //
    if (sig >= SIGNAL_TABLE_SIZE_IN_SLOTS)    die ("interprocess-signals.c: c_signal_handler: sig d=%d >= SIGNAL_TABLE_SIZE_IN_SLOTS %d\n", sig, SIGNAL_TABLE_SIZE_IN_SLOTS ); 

    hostthread->posix_signal_counts[code].seen_count++;
    hostthread->all_posix_signals.seen_count++;

    hostthread->ccall_limit_pointer_mask = 0;

    if (hostthread->executing_mythryl_code
    &&  !hostthread->posix_signal_pending
    &&  !hostthread->mythryl_handler_for_posix_signal_is_running
    ){
       hostthread->posix_signal_pending = TRUE;
       ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER();
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
    extern Vunt request_fault [];

    Task* task = SELF_HOSTTHREAD->task;

    int code = exn->ExceptionCode;

    DWORD pc = (DWORD) exn->ExceptionAddress;

    if (*SELF_HOSTTHREAD->executing_mythryl_code) {
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

void   run_mythryl_task_and_runtime_eventloop__may_heapclean   (Task* task, Roots* extra_roots) {
    // ================
    // This overrides the default RunLib7.  
    // It just adds a new exception handler
    // at the very beginning before
    // Mythryl code is executed.

     extern void system_run_mythryl_task_and_runtime_eventloop__may_heapclean (Task*,Roots*);		// system_run_mythryl_task_and_runtime_eventloop__may_heapclean		def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c

     exception_list el;
     el.handler = page_fault_handler;
     el.prev    = _win32_exception_list;
     _win32_exception_list = &el;

     return system_run_mythryl_task_and_runtime_eventloop__may_heapclean(task,extra_roots);
}

#endif
