/* runtime-state.h
 *
 * This is the C view of the state of a Lib7 computation.
 */

#ifndef _LIB7_STATE_
#define _LIB7_STATE_

#ifndef _LIB7_BASE_
#include "runtime-base.h"
#endif

#ifndef _LIB7_ROOTS_
#include "runtime-roots.h"
#endif

#if (!defined(_CNTR_) && defined(ICOUNT))
#include "cntr.h"
#endif

#define CALLEESAVE	3


/** The Lib7 state vector **
 */
/* typedef struct lib7_state lib7_state_t; */  /* defined in runtime-base.h */
struct lib7_state {
				    /* Lib7 task info */
#   define      lib7_allocArena     lib7_heap->allocBase
#   define	lib7_allocArenaSzB  lib7_heap->allocSzB

    heap_t*		lib7_heap;	    /* The heap for this Lib7 task */
    vproc_state_t* 	lib7_vproc;	    /* the VProc that this is running on */

				/* Lib7 registers */
    lib7_val_t*	lib7_heap_cursor;
    lib7_val_t*	lib7_heap_limit;
    lib7_val_t		lib7_argument;
    lib7_val_t		lib7_fate;
    lib7_val_t		lib7_closure;
    lib7_val_t		lib7_link_register;
    lib7_val_t		lib7_program_counter;		    /* Address of Lib7 code to execute; when     */
							    /* calling a Lib7 frunction from C, this     */
							    /* holds the same value as the link_register. */
    lib7_val_t		lib7_exception_fate;
    lib7_val_t		lib7_current_thread;
    lib7_val_t		lib7_calleeSave[CALLEESAVE];

    lib7_val_t		lib7_store_log;  /* The list of store operations. */

    lib7_val_t		lib7_fault_exception;		/* The exception packet for a hardware fault. */
    Word_t		lib7_faulting_program_counter;	/* the PC of the faulting instruction */

#ifdef SOFT_POLL
    lib7_val_t*	lib7_real_heap_limit;          /* The real heap limit. */
    bool_t      	lib7_poll_event_is_pending;    /* Is a poll event pending? */
    bool_t      	lib7_in_poll_handler;          /* Are we currently executing inside a poll handler? */
#endif
};


/* Set up the return linkage and fate
 * throwing in the Lib7 state vector:
 */
#define SETUP_RETURN(lib7_state)	{						\
	    lib7_state_t*	__lib7_state    = (lib7_state);				\
	    __lib7_state->lib7_closure	 = LIB7_void;				\
	    __lib7_state->lib7_program_counter = __lib7_state->lib7_fate;		\
	}

#define SETUP_THROW(lib7_state, fate, val)	{				\
	    lib7_state_t	*__lib7_state	= (lib7_state);			\
	    lib7_val_t	__fate	= (fate);				\
	    __lib7_state->lib7_closure	= __fate;			\
	    __lib7_state->lib7_fate= LIB7_void;				\
	    __lib7_state->lib7_program_counter	=				\
	    __lib7_state->lib7_link_register	= GET_CODE_ADDR( __fate );	\
	    __lib7_state->lib7_exception_fate	= LIB7_void;			\
	    __lib7_state->lib7_argument	= (val);					\
	}

#endif /* !_LIB7_STATE_ */



/* COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

