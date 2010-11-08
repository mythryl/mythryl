/* win32-fault.c
 *
 * win32 code for handling traps (arithmetic overflow, div-by-0, ctrl-c, etc.).
 */

#include "../config.h"

#include <windows.h>
#include <excpt.h>

#include "runtime-base.h"
#include "vproc-state.h"
#include "runtime-state.h"
#include "runtime-globals.h"
#include "signal-sysdep.h"
#include "system-signals.h"

#include "win32-fault.h"

#define SELF_VPROC (VProc[0])

/* globals */
HANDLE win32_stdin_handle;
HANDLE win32_console_handle;
HANDLE win32_stdout_handle;
HANDLE win32_stderr_handle;

HANDLE win32_LIB7_thread_handle;
BOOL win32_isNT;

/* static globals */
static BOOL caught_cntrl_c = FALSE;

void wait_for_cntrl_c()
{
  /* we know a cntrl_c is coming; wait for it */
  while (!caught_cntrl_c)
    ;
}

/* generic handler for win32 "signals" such as interrupt, alarm */
/* returns TRUE if the main thread is running Lib7 code */
BOOL win32_generic_handler(int code)
{
    vproc_state_t   *vsp = SELF_VPROC;

    vsp->vp_sigCounts[code].nReceived++;
    vsp->vp_totalSigCount.nReceived++;

    vsp->vp_limitPtrMask = 0;

    if (vsp->vp_inLib7Flag && 
      (*vsp->vp_handlerPending) && 
      (*vsp->vp_inSigHandler))
    {
	vsp->vp_handlerPending = TRUE;
	SIG_ZeroLimitPtr();
	return TRUE;
    }
    return FALSE;
}

/* cntrl_c_handler
 * the win32 handler for ctrl-c
 */
static
BOOL cntrl_c_handler(DWORD fdwCtrlType) 
{
  int ret = FALSE;

  /* SayDebug("event is %x\n", fdwCtrlType); */
  switch (fdwCtrlType) {
    case CTRL_BREAK_EVENT:
    case CTRL_C_EVENT: {
      if (!win32_generic_handler(SIGINT)) {
	caught_cntrl_c = TRUE;
      }
      ret = TRUE;  /* we handled the event */
      break;
    }
  }
  return ret;  /* chain to other handlers */
}


/* set_up_fault_handlers:
 */
void set_up_fault_handlers (lib7_state_t *lib7_state)
{
  /* some basic win32 initialization is done here */

  /* determine if we're NT or 95 */
  win32_isNT = !(GetVersion() & 0x80000000);

  /* get the redirected handle; this is "stdin"  */
  win32_stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
  /* get the actual handle of the console */
  win32_console_handle = CreateFile("CONIN$",
				    GENERIC_READ|GENERIC_WRITE,
				    FILE_SHARE_READ|FILE_SHARE_WRITE,
				    NULL,
				    OPEN_EXISTING,
				    0,0);
#ifdef WIN32_DEBUG
  if (win32_console_handle == INVALID_HANDLE_VALUE) {
    SayDebug("win32: failed to get actual console handle");
  }
#endif

  win32_stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
  win32_stderr_handle = GetStdHandle(STD_ERROR_HANDLE);

#ifdef WIN32_DEBUG
  SayDebug("console input handle, %x\n", (unsigned int) win32_stdin_handle);
  SayDebug("console output handle, %x\n", (unsigned int) win32_stdout_handle);
  SayDebug("console error handle, %x\n", (unsigned int) win32_stderr_handle);
#endif

  /* create a thread id for the main thread */
  {
    HANDLE cp_h = GetCurrentProcess();

    if (!DuplicateHandle(cp_h,               /* process with handle to dup */
			 GetCurrentThread(), /* pseudohandle, hence the dup */
			 cp_h,               /* handle goes to current proc */
			 &win32_LIB7_thread_handle, /* recipient */
			 THREAD_ALL_ACCESS,
			 FALSE,
			 0                   /* no options */
			 )) {
      Die ("win32:set_up_fault_handlers: cannot duplicate thread handle");
    }
  }
  
  /* install the ctrl-C handler */
  if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)cntrl_c_handler,TRUE)) {
    Die("win32:set_up_fault_handlers: can't install cntrl_c_handler\n");
  }

  /* initialize the floating-point unit */
  SIG_InitFPE ();
}

static bool_t fault_handler (int code, Word_t pc)
{
  lib7_state_t	    *lib7_state = SELF_VPROC->vp_state;
  extern Word_t request_fault[];

  if (*SELF_VPROC->vp_inLib7Flag) 
    Die ("win32:fault_handler: bogus fault not in Lib7: %#x\n", code);

  /* Map the signal to the appropriate Lib7 exception. */
  switch (code) {
    case EXCEPTION_INT_DIVIDE_BY_ZERO: 
      lib7_state->lib7_fault_exception = DivId;
      lib7_state->lib7_faulting_program_counter = pc;
      break;
    case EXCEPTION_INT_OVERFLOW:
      lib7_state->lib7_fault_exception = OverflowId;
      lib7_state->lib7_faulting_program_counter = pc;
      break;
    default:
      Die ("win32:fault_handler: unexpected fault @%#x, code = %#x", pc, code);
  }
  return TRUE;
}

/* restoreregs
 * this is where win32 handles traps
 */
int restoreregs(lib7_state_t *lib7_state)
{
  extern Word_t request_fault[];

  caught_cntrl_c = FALSE;
  __try{
    int request;

    request = asm_restoreregs(lib7_state);
    return request;

  } __except(fault_handler(GetExceptionCode(), (Word_t *)(GetExceptionInformation())->ContextRecord->Eip) ?
#ifdef HOST_X86
	     ((Word_t *)(GetExceptionInformation())->ContextRecord->Eip = request_fault,
              EXCEPTION_CONTINUE_EXECUTION) :
	      EXCEPTION_CONTINUE_SEARCH)
#else
#  error  non-x86 win32 platforms need restoreregs support
#endif
  { /* nothing */ }
}

/* end of win32-fault.c */


/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

