/* getpquantum.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "cfun-proto-list.h"
#include "profile.h"

/* _lib7_Prof_getpquantum : Void -> int
 *
 * Return the profile timer quantim in microseconds.
 */
lib7_val_t _lib7_Prof_getpquantum (lib7_state_t *lib7_state, lib7_val_t arg)
{
    return INT_CtoLib7(PROFILE_QUANTUM_US);

} /* end of _lib7_Prof_getpquantum */


/* COPYRIGHT (c) 1996 AT&T Research.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
