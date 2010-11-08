/* record-ops.c
 *
 * Some (type unsafe) operations on records.
 */

/* ###       "Old C programmers never die,
   ###        they just get cast into void."
   ###                    -- Anonymous
 */

#include "../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-state.h"
#include "runtime-heap.h"
#include "arena-id.h"
#include "gc.h"


/* GetLen:
 *
 * Check that we really have a record chunk, and return its length.
 */
static int GetLen (lib7_val_t r)
{
    lib7_val_t	d;
    int		t;

    if (! isBOXED(r))
	return -1;

    switch (EXTRACT_CHUNKC(ADDR_TO_PAGEID(BIBOP, r))) {
      case CHUNKC_new:
	d = CHUNK_DESC(r);
	t = GET_TAG(d);
	if (t == DTAG_record)
	    return GET_LEN(d);
	else
	    return -1;
      case CHUNKC_pair: return 2;
      case CHUNKC_record:
	d = CHUNK_DESC(r);
	t = GET_TAG(d);
	if (t == DTAG_record)
	    return GET_LEN(d);
	else
	    return -1;
      default:
	return -1;
    }

}

/* RecordMeld:
 *
 * Concatenate two records; returns unit if either argument is not
 * a record of length at least one.
 */
lib7_val_t RecordMeld (lib7_state_t *lib7_state, lib7_val_t r1, lib7_val_t r2)
{
    int		l1 = GetLen(r1);
    int		l2 = GetLen(r2);

    if ((l1 > 0) && (l2 > 0)) {
	int		n = l1+l2;
	int		i, j;
	lib7_val_t	*p, res;

	LIB7_AllocWrite (lib7_state, 0, MAKE_DESC(n, DTAG_record));
	j = 1;
	for (i = 0, p = PTR_LIB7toC(lib7_val_t, r1);  i < l1;  i++, j++) {
	    LIB7_AllocWrite (lib7_state, j, p[i]);
	}
	for (i = 0, p = PTR_LIB7toC(lib7_val_t, r2);  i < l2;  i++, j++) {
	    LIB7_AllocWrite (lib7_state, j, p[i]);
	}
	res = LIB7_Alloc(lib7_state, n);
	return res;
    }
    else {
	return LIB7_void;
    }

} /* end of RecordMeld */



/* COPYRIGHT (c) 1998 Bell Labs, Lucent Technologies.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

