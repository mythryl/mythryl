/*
 * Special signal handling for cygwin on Windows.
 *
 * Even though cygwin behaves like "unix", its signal handling mechanism
 * is crippled.  I haven't been able to get/set the EIP addresses from
 * the siginfo_t and related data structures.  So here I'm using 
 * Windows and some gcc assembly hacks to get things done. 
 */


#if defined(__i386__) && defined(__CYGWIN32__) && defined(__GNUC__)

#include "../config.h"

#include "runtime-unixdep.h"
#include "signal-sysdep.h"
#include "runtime-base.h"
#include "vproc-state.h"
#include "runtime-state.h"
#include "runtime-globals.h"

#include <windows.h>
#include <exceptions.h> /* Cygwin stuff */

#define SELF_VPROC      (VProc[0])

/* generic handler for cygwin "signals" such as interrupt, alarm */
/* returns TRUE if the main thread is running Lib7 code */
BOOL cygwin_generic_handler(int code)
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

static BOOL __stdcall ctrl_c_handler(DWORD type)
{
   switch (type)
   {
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

void set_up_fault_handlers(lib7_state_t * lib7_state)
{
   /* Install the control-C handler */
   if (*SetConsoleCtrlHandler(ctrl_c_handler, TRUE))
   {
      Die("cygwin:set_up_fault_handlers: can't install ctrl-c-handler\n");
   }
   /* Initialize the floating-point unit */
   SIG_InitFPE ();
}

/*
 * This filter catches all exceptions. 
 */
static int page_fault_handler
   (EXCEPTION_RECORD * exn, void * foo, CONTEXT * c, void * bar)
{
   extern Word_t request_fault[];
   lib7_state_t * lib7_state = SELF_VPROC->vp_state;
   int code = exn->ExceptionCode;
   DWORD pc = (DWORD)exn->ExceptionAddress;

   if (*SELF_VPROC->vp_inLib7Flag)
   {
      Die("cygwin:fault_handler: bogus fault not in Lib7: %#x\n", code);
   }

   switch (code)
   {
      case EXCEPTION_INT_DIVIDE_BY_ZERO:
         /* say("Divide by zero at %p\n", pc); */
         lib7_state->lib7_fault_exception = DivId;
         lib7_state->lib7_faulting_program_counter  = pc;
         c->Eip = (DWORD)request_fault;
         break;
      case EXCEPTION_INT_OVERFLOW:
         /* say("OVERFLOW at %p\n", pc); */
         lib7_state->lib7_fault_exception = OverflowId;
         lib7_state->lib7_faulting_program_counter  = pc;
         c->Eip = (DWORD)request_fault;
         break;
      default:
         Die("cygwin:fault_handler: unexpected fault @%#x, code=%#x", pc, code);
   }
   return FALSE;
}

asm (".equ __win32_exception_list,0");
extern exception_list * 
   _win32_exception_list asm ("%fs:__win32_exception_list");

/*
 * This overrides the default RunLib7.  
 * It just adds a new exception handler at the very beginning before
 * Lib7 is executed.
 */
void RunLib7(lib7_state_t * lib7_state)
{
   extern void SystemRunLib7(lib7_state_t *);

   exception_list el;
   el.handler = page_fault_handler;
   el.prev    = _win32_exception_list;
   _win32_exception_list = &el;
   return SystemRunLib7(lib7_state);
}

#endif
