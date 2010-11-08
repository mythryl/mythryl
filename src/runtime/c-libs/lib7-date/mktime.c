/* mktime.c
 *
 */

#include "../../config.h"

#include <time.h>
#include "runtime-base.h"
#include "lib7-c.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"

/* _lib7_Date_mktime : (Int, Int, Int, Int, Int, Int, Int, Int, Int)
 *	-> int32::Int
 *
 * This takes a 9-tuple with the fields: tm_sec, tm_min, tm_hour, tm_mday,
 * tm_mon, tm_year, tm_wday, tm_yday, tm_isdst, and returns the corresponding
 * localtime value (in seconds).
 */
lib7_val_t _lib7_Date_mktime (lib7_state_t *lib7_state, lib7_val_t arg)
{
    struct tm	tm;
    time_t	t;

    tm.tm_sec	= REC_SELINT(arg, 0);
    tm.tm_min	= REC_SELINT(arg, 1);
    tm.tm_hour	= REC_SELINT(arg, 2);
    tm.tm_mday	= REC_SELINT(arg, 3);
    tm.tm_mon	= REC_SELINT(arg, 4);
    tm.tm_year	= REC_SELINT(arg, 5);
    /* tm.tm_wday = REC_SELINT(arg, 6); */  /* ignored by mktime */
    /* tm.tm_yday = REC_SELINT(arg, 7); */  /* ignored by mktime */
    tm.tm_isdst	= REC_SELINT(arg, 8);

    t = mktime (&tm);

    if (t < 0) {
        return RAISE_ERROR(lib7_state, "Invalid date");
    }
    else {
	lib7_val_t result;

	INT32_ALLOC(lib7_state, result, t);

	return result;
    }
}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
