/* runtime-signals.h
 *
 */

#ifndef _LIB7_SIGNALS_
#define _LIB7_SIGNALS_

typedef struct {		/* counters for pending signals; we keep two counters */
				/* to avoid race conditions */
    unsigned int	nReceived;  /* the count of how many signals of this variety */
				    /* have been received. This counter is incremented */
				    /* the signal handler */
    unsigned int	nHandled;  /* the count of how many of this kind of */
				    /* signal have been handled.  This counter */
				    /* is incremented by the main thread. */
} sig_count_t;

/* The state of Lib7 signal handlers; these definitions must agree with
 * the values used in signals.pkg.
 */
#define LIB7_SIG_IGNORE		0
#define LIB7_SIG_DEFAULT		1
#define LIB7_SIG_ENABLED		2

/** Utility functions **/
extern void ChooseSignal (vproc_state_t *vsp);
extern lib7_val_t MakeResumeCont (lib7_state_t *lib7_state, lib7_val_t resume[]);
extern lib7_val_t MakeHandlerArg (lib7_state_t *lib7_state, lib7_val_t resume[]);
extern void LoadResumeState (lib7_state_t *lib7_state);

/* OS dependent implementations of signal operations. */
extern lib7_val_t ListSignals (lib7_state_t *lib7_state);
extern void PauseUntilSignal (vproc_state_t *vsp);
extern void SetSignalState (vproc_state_t *vsp, int sigNum, int sigState);
extern int GetSignalState (vproc_state_t *vsp, int sigNum);
extern void SetSignalMask (lib7_val_t sigList);
extern lib7_val_t GetSignalMask (lib7_state_t *lib7_state);

#endif /* !_LIB7_SIGNALS_ */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
