/* record-cat.c
 *
 * Concatenation for records.
 */


#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-state.h"
#include "runtime-heap.h"
#include "gc.h"
#include "cfun-proto-list.h"
#include "lib7-c.h"



lib7_val_t   _lib7_runtime_recordmeld   (   lib7_state_t*   lib7_state,
                                                lib7_val_t      arg
                                            )
{
    /* _lib7_runtime_recordmeld : (chunk * chunk) -> chunk */

    lib7_val_t    r1 = REC_SEL(arg,0);
    lib7_val_t    r2 = REC_SEL(arg,1);

    if (r1 == LIB7_void)	return r2;
    else if (r2 == LIB7_void)	return r1;
    else {
	lib7_val_t	result = RecordMeld (lib7_state, r1, r2);

	if (result == LIB7_void)   return RAISE_ERROR( lib7_state, "recordmeld: not a record");
	else                       return result;
    }
}



/* COPYRIGHT (c) 1998 Bell Labs, Lucent Technologies.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

