/* setpref.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "lib7-c.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "runtime-globals.h"
#include "cfun-proto-list.h"
#include "profile.h"

extern void EnableProfSignals (void);
extern void DisableProfSignals (void);

/* _lib7_Prof_setpref : Null_Or( Rw_Vector( Unt ) ) -> Void
 *
 * Set the profile array reference; NULL means that there is no array.
 */
lib7_val_t _lib7_Prof_setpref (lib7_state_t *lib7_state, lib7_val_t arg)
{
#ifdef OPSYS_UNIX
    bool_t	enabled = (ProfCntArray != LIB7_void);
    int		i;

    if (arg != OPTION_NONE) {
	ProfCntArray = OPTION_get(arg);
	if (! enabled) {
	  /* add ProfCntArray to the C roots */
	    CRoots[NumCRoots++] = &ProfCntArray;
	  /* enable profiling signals */
	    EnableProfSignals ();
	}
    }
    else if (enabled) {
      /* remove ProfCntArray from the C roots */
	for (i = 0;  i < NumCRoots;  i++) {
	    if (CRoots[i] == &ProfCntArray) {
		CRoots[i] = CRoots[--NumCRoots];
		break;
	    }
	}
      /* disable profiling signals */
	DisableProfSignals ();
	ProfCntArray = LIB7_void;
    }

    return LIB7_void;
#else
    return RAISE_ERROR(lib7_state, "time profiling not supported");
#endif

} /* end of _lib7_Prof_setpref */


/* COPYRIGHT (c) 1996 AT&T Research.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
