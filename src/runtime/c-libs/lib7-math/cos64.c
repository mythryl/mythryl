/* cos64.c
 *
 */

#include "../../config.h"

#include <math.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"

/* _lib7_Math_cos64:
 */
lib7_val_t _lib7_Math_cos64 (lib7_state_t *lib7_state, lib7_val_t arg)
{
    double		d = *(PTR_LIB7toC(double, arg));
    lib7_val_t		res;

    REAL64_ALLOC(lib7_state, res, cos(d));

    return res;

} /* end of _lib7_Math_cos64 */


/* COPYRIGHT (c) 1996 AT&T Research.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
