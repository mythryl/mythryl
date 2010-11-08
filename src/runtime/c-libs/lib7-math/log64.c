/* log64.c
 *
 */

#include "../../config.h"

#include <math.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"

/* _lib7_Math_log64:
 */
lib7_val_t _lib7_Math_log64 (lib7_state_t *lib7_state, lib7_val_t arg)
{
    double		d = *(PTR_LIB7toC(double, arg));

    lib7_val_t		result;

    REAL64_ALLOC(lib7_state, result, log(d));

    return result;

} /* end of _lib7_Math_log64 */


/* COPYRIGHT (c) 1996 AT&T Research.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
