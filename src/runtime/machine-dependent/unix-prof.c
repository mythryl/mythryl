/* unix-prof.c
 *
 * Lib7 Profiling support for Unix.
 */

#include "../config.h"

#include "runtime-unixdep.h"
#include "signal-sysdep.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "runtime-globals.h"
#include "profile.h"


/* The pointer to the heap allocated rw_vector of call counts.
 * When this pointer is LIB7_void, then profiling is disabled.
 */
lib7_val_t	ProfCntArray = LIB7_void;

/* local routines */
static SigReturn_t ProfSigHandler ();


/* EnableProfSignals:
 */
void EnableProfSignals ()
{
    SIG_SetHandler (SIGVTALRM, ProfSigHandler);

} /* end of EnableProfSignals */

/* DisableProfSignals:
 */
void DisableProfSignals ()
{
    SIG_SetHandler (SIGVTALRM, SIG_DFL);

} /* end of DisableProfSignals */

/* ProfSigHandler:
 *
 * The handler for SIGVTALRM signals.
 */
static SigReturn_t ProfSigHandler ()
{
    Word_t	*arr = GET_SEQ_DATAPTR(Word_t, ProfCntArray);
    int		index = INT_LIB7toC(DEREF(ProfCurrent));

    arr[index]++;

} /* end of ProfSigHandler */



/* COPYRIGHT (c) 1996 AT&T Research.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

