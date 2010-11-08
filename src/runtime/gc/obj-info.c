/* chunk-info.c
 *
 */

#include "../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "heap.h"
#include "gc.h"

/* GetChunkGen:
 *
 * Get the generation of an chunk (return -1 for external/unboxed chunks).
 */
int GetChunkGen (lib7_val_t chunk)
{
    if (isBOXED(chunk)) {
	aid_t	aid = ADDR_TO_PAGEID(BIBOP, chunk);
	if (IS_BIGCHUNK_AID(aid)) {
	    int		i;
	    bigchunk_region_t	*region;
	    bigchunk_desc_t	*dp;

	    for (i = BIBOP_ADDR_TO_INDEX(chunk);  !BO_IS_HDR(aid);  aid = BIBOP[--i])
		continue;
	    region = (bigchunk_region_t *)BIBOP_INDEX_TO_ADDR(i);
	    dp = ADDR_TO_BODESC(region, chunk);

	    return dp->gen;
	}
	else if (aid == AID_NEW)
	    return 0;
	else if (isUNMAPPED(aid))
	    return -1;
	else
	    return EXTRACT_GEN(aid);
    }
    else
	return -1;

} /* end of GetChunkGen */



/* COPYRIGHT (c) 1993 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
