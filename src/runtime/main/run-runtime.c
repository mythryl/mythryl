/* run-runtime.c
 *
 */

#include "../config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "runtime-base.h"
#include "runtime-limits.h"
#include "runtime-values.h"
#include "vproc-state.h"
#include "runtime-state.h"
#include "tags.h"
#include "runtime-request.h"
#include "runtime-heap.h"
#include "runtime-globals.h"
#include "runtime-signals.h"
#include "c-library.h"
#include "profile.h"
#include "gc.h"

/* local functions */
static void UncaughtExn (lib7_val_t e);


/* ApplyLib7Fn:
 *
 * Apply the Lib7 closure f to arg and return the result.  If the flag useCont
 * is set, then the Lib7 state has already been initialized with a return
 * fate (by SaveCState).
 */
lib7_val_t ApplyLib7Fn (lib7_state_t *lib7_state, lib7_val_t f, lib7_val_t arg, bool_t useCont)
{
    InitLib7state (lib7_state);

    /* initialize the calling context */
    lib7_state->lib7_exception_fate	= PTR_CtoLib7(handle_v+1);
    lib7_state->lib7_current_thread     = LIB7_void;
    lib7_state->lib7_argument		= arg;
    if (! useCont) {
	lib7_state->lib7_fate	= PTR_CtoLib7(return_c);
    }
    lib7_state->lib7_closure	= f;
    lib7_state->lib7_program_counter		=
    lib7_state->lib7_link_register	= GET_CODE_ADDR(f);

    RunLib7 (lib7_state);

    return lib7_state->lib7_argument;

} /* end of ApplyLib7Fn */


/* RaiseLib7Exception:
 *
 * Modify the Lib7 state so that the given exception
 * will be raised when Lib7 is resumed.
 */
void  RaiseLib7Exception  (lib7_state_t *lib7_state, lib7_val_t exn)
{
    lib7_val_t	kont = lib7_state->lib7_exception_fate;

    /* We should have a macro defined in runtime-state.h for this.  XXX BUGGO FIXME **/

    lib7_state->lib7_argument	= exn;
    lib7_state->lib7_closure	= kont;
    lib7_state->lib7_fate	= LIB7_void;

    lib7_state->lib7_program_counter	=
    lib7_state->lib7_link_register	= GET_CODE_ADDR(kont);

} /* end of RaiseLib7Exception. */

extern int restoreregs (lib7_state_t *lib7_state);

/* RunLib7:
 */
#if defined(__CYGWIN32__)
void SystemRunLib7 (lib7_state_t *lib7_state)
#else
void RunLib7 (lib7_state_t *lib7_state)
#endif
{
    int		request;
    vproc_state_t *vsp = lib7_state->lib7_vproc;
    lib7_val_t	prevProfIndex = PROF_OTHER;

    for (;;) {
						/* ProfCurrent	is #defined in   src/runtime/include/runtime-globals.h   in terms of   _ProfCurrent   from   src/runtime/main/globals.c	*/
						/* PROF_RUNTIME	is #defined in   src/runtime/include/profile.h	*/
	ASSIGN(ProfCurrent, prevProfIndex);
	request = restoreregs(lib7_state);
	prevProfIndex = DEREF(ProfCurrent);
	ASSIGN(ProfCurrent, PROF_RUNTIME);

	if (request == REQ_GC) {

	    if (vsp->vp_handlerPending) {

		/* This is really a UNIX signal */

		if (need_to_collect_garbage (lib7_state, 4*ONE_K))
		    collect_garbage (lib7_state, 0);

	        /* Figure out which unix signal needs handling
		 * and save its (number, count) in (vsp->vp_sigCode, vsp->vp_sigCount).
		 *
		 * Choose_Signal() and MakeHandlerArg() are both from
		 *
		 *     src/runtime/machine-dependent/signal-util.c
		 *
		 * Our actual kernel-invoked signal handler is   CSigHandler()   from
		 *
		 *     src/runtime/machine-dependent/unix-signal.c
		 *
		 * Lib7SignalHandler in practice points to   signal_handler()   from
                 *
                 *     src/lib/std/src/nj/internal-signals.pkg
                 *
 		 * sigh_resume  is assembly code from
		 * (depending upon platform) one of:
		 *     src/runtime/machine-dependent/X86.prim.asm
		 *     src/runtime/machine-dependent/X86.prim.masm
                 *     src/runtime/machine-dependent/SPARC.prim.asm
                 *     src/runtime/machine-dependent/PPC.prim.asm
		 *
                 * sigh_return_c appears to be set nowhere; it may be obsoleted garbage.
                 */
		ChooseSignal( vsp );
		/**/
		lib7_state->lib7_argument	 = MakeHandlerArg( lib7_state, sigh_resume );
		lib7_state->lib7_fate	         = PTR_CtoLib7(sigh_return_c);
		lib7_state->lib7_exception_fate	 = PTR_CtoLib7(handle_v+1);
		lib7_state->lib7_closure	 = DEREF(Lib7SignalHandler);
		/**/
		lib7_state->lib7_program_counter =
		lib7_state->lib7_link_register	 = GET_CODE_ADDR(lib7_state->lib7_closure);
		/**/
		vsp->vp_inSigHandler	= TRUE;
		vsp->vp_handlerPending	= FALSE;
	    }
#ifdef SOFT_POLL
	    else if (lib7_state->lib7_poll_event_is_pending && !lib7_state->lib7_in_poll_handler) { 
	      /* this is a poll event */
#if defined(MP_SUPPORT) && defined(MP_GCPOLL)
	      /* Note: under MP, polling is used for GC only */
#ifdef POLL_DEBUG
SayDebug ("run-runtime: poll event\n");
#endif
	        lib7_state->lib7_poll_event_is_pending = FALSE;
	        collect_garbage (lib7_state,0);
#else
		if (need_to_collect_garbage (lib7_state, 4*ONE_K)) {
		    collect_garbage (lib7_state, 0);
                }
		lib7_state->lib7_argument	= MakeResumeCont(lib7_state, pollh_resume);	/* MakeResumeCont is from  src/runtime/machine-dependent/signal-util.c */
		lib7_state->lib7_fate		= PTR_CtoLib7(pollh_return_c);
		lib7_state->lib7_exception_fate	= PTR_CtoLib7(handle_v+1);
		lib7_state->lib7_closure	= DEREF(Lib7PollHandler);
		/**/
		lib7_state->lib7_program_counter=
		lib7_state->lib7_link_register	= GET_CODE_ADDR(lib7_state->lib7_closure);
		/**/
		lib7_state->lib7_in_poll_handler= TRUE;
		lib7_state->lib7_poll_event_is_pending	= FALSE;
#endif /* MP_SUPPORT */
	    } 
#endif /* SOFT_POLL */
	    else
	        collect_garbage (lib7_state, 0);
	}
	else {
	    switch (request) {
	      case REQ_RETURN:
	        /* do a minor collection to clear the store list */
		collect_garbage (lib7_state, 0);
		return;

	      case REQ_EXN: /* an UncaughtExn exception */
		UncaughtExn (lib7_state->lib7_argument);
		return;

	      case REQ_FAULT: { /* a hardware fault */
		    lib7_val_t	loc, traceStk, exn;
		    char *namestring;
		    if ((namestring = BO_AddrToCodeChunkTag(lib7_state->lib7_faulting_program_counter)) != NULL)
		    {
			char	buf2[192];
			sprintf(buf2, "<file %.184s>", namestring);
			loc = LIB7_CString(lib7_state, buf2);
		    }
		    else
			loc = LIB7_CString(lib7_state, "<unknown file>");
		    LIST_cons(lib7_state, traceStk, loc, LIST_nil);
		    EXN_ALLOC(lib7_state, exn, lib7_state->lib7_fault_exception, LIB7_void, traceStk);
		    RaiseLib7Exception (lib7_state, exn);
		} break;

	      case REQ_BIND_CFUN:
		lib7_state->lib7_argument = BindCFun (
		    STR_LIB7toC(REC_SEL(lib7_state->lib7_argument, 0)),
		    STR_LIB7toC(REC_SEL(lib7_state->lib7_argument, 1)));
		SETUP_RETURN(lib7_state);
		break;

	      case REQ_CALLC: {
		    lib7_val_t    (*f)(), arg;

		    SETUP_RETURN(lib7_state);
		    if (need_to_collect_garbage (lib7_state, 8*ONE_K))
			collect_garbage (lib7_state, 0);

#ifdef INDIRECT_CFUNC
		    f = ((cfunc_naming_t *)REC_SELPTR(Word_t, lib7_state->lib7_argument, 0))->cfunc;
#  ifdef DEBUG_TRACE_CCALL
		    SayDebug("CALLC: %s (%#x)\n",
			((cfunc_naming_t *)REC_SELPTR(Word_t, lib7_state->lib7_argument, 0))->name,
			REC_SEL(lib7_state->lib7_argument, 1));
#  endif
#else
		    f = (cfunc_t) REC_SELPTR(Word_t, lib7_state->lib7_argument, 0);
#  ifdef DEBUG_TRACE_CCALL
		    SayDebug("CALLC: %#x (%#x)\n", f, REC_SEL(lib7_state->lib7_argument, 1));
#  endif
#endif
		    arg = REC_SEL(lib7_state->lib7_argument, 1);
		    lib7_state->lib7_argument = (*f)(lib7_state, arg);
		} break;

	      case REQ_ALLOC_STRING:
		lib7_state->lib7_argument = LIB7_AllocString (lib7_state, INT_LIB7toC(lib7_state->lib7_argument));
		SETUP_RETURN(lib7_state);
		break;

	      case REQ_ALLOC_BYTEARRAY:
		lib7_state->lib7_argument = LIB7_AllocBytearray (lib7_state, INT_LIB7toC(lib7_state->lib7_argument));
		SETUP_RETURN(lib7_state);
		break;

	      case REQ_ALLOC_REALDARRAY:
		lib7_state->lib7_argument = LIB7_AllocRealdarray (lib7_state, INT_LIB7toC(lib7_state->lib7_argument));
		SETUP_RETURN(lib7_state);
		break;

	      case REQ_ALLOC_ARRAY:
		lib7_state->lib7_argument = LIB7_AllocArray (lib7_state,
		    REC_SELINT(lib7_state->lib7_argument, 0), REC_SEL(lib7_state->lib7_argument, 1));
		SETUP_RETURN(lib7_state);
		break;

	      case REQ_ALLOC_VECTOR:
		lib7_state->lib7_argument = LIB7_AllocVector (lib7_state,
		    REC_SELINT(lib7_state->lib7_argument, 0), REC_SEL(lib7_state->lib7_argument, 1));
		SETUP_RETURN(lib7_state);
		break;

	      case REQ_SIG_RETURN:
#ifdef SIGNAL_DEBUG
SayDebug("REQ_SIG_RETURN: arg = %#x, pending = %d, inHandler = %d, nSigs = %d/%d\n",
lib7_state->lib7_argument, vsp->vp_handlerPending, vsp->vp_inSigHandler,
vsp->vp_totalSigCount.nHandled, vsp->vp_totalSigCount.nReceived);
#endif
	        /* Throw to the fate: */
		SETUP_THROW( lib7_state, lib7_state->lib7_argument, LIB7_void );

	        /* Note that we are exiting the handler: */
		vsp->vp_inSigHandler = FALSE;
		break;

#ifdef SOFT_POLL
	      case REQ_POLL_RETURN:
	        /* Throw to the fate: */
		SETUP_THROW( lib7_state, lib7_state->lib7_argument, LIB7_void );

	        /* Note that we are exiting the handler: */
		lib7_state->lib7_in_poll_handler = FALSE;
		ResetPollLimit( lib7_state );
		break;
#endif

#ifdef SOFT_POLL
	      case REQ_POLL_RESUME:
#endif
	      case REQ_SIG_RESUME:
#ifdef SIGNAL_DEBUG
SayDebug("REQ_SIG_RESUME: arg = %#x\n", lib7_state->lib7_argument);
#endif
		LoadResumeState (lib7_state);
		break;

	      case REQ_BUILD_LITERALS:
		Die ("BUILD_LITERALS request");
		break;

	      default:
		Die ("unknown request code = %d", request);
		break;
	    } /* end switch */
	}
    } /* end of while */

} /* end of RunLib7 */


/* UncaughtExn:
 * Handle an uncaught exception.
 */
static void UncaughtExn (lib7_val_t e)
{
    lib7_val_t	name = REC_SEL(REC_SEL(e, 0), 0);
    lib7_val_t	val = REC_SEL(e, 1);
    lib7_val_t	traceBack = REC_SEL(e, 2);
    char	buf[1024];

    if (isUNBOXED(val))
	sprintf (buf, "%ld\n", (long int) INT_LIB7toC(val));
    else {
	lib7_val_t	desc = CHUNK_DESC(val);
	if (desc == DESC_string)
	    sprintf (buf, "\"%.*s\"", (int) GET_SEQ_LEN(val), STR_LIB7toC(val));
	else
	    sprintf (buf, "<unknown>");
    }

    if (traceBack != LIST_nil) {
      /* find the information about where this exception was raised */
	lib7_val_t	next = traceBack;
	do {
	    traceBack = next;
	    next = LIST_tl(traceBack);
	} while (next != LIST_nil);
	val = LIST_hd(traceBack);
	sprintf (buf+strlen(buf), " raised at %.*s",
		 (int) GET_SEQ_LEN(val), STR_LIB7toC(val));
    }

    Die ("Uncaught exception %.*s with %s\n",
	GET_SEQ_LEN(name), GET_SEQ_DATAPTR(char, name), buf);

    Exit (1);

} /* end of UncaughtExn */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
