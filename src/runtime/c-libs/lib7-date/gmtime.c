/* gmtime.c
 *
 */

#include "../../config.h"

#include <time.h>
#include "runtime-base.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"
#include "lib7-c.h"

/* _lib7_Date_gmtime : int32::Int -> (Int, Int, Int, Int, Int, Int, Int, Int, Int)
 *
 * Takes a UTC time value (in seconds), and converts it to a 9-tuple with
 * the fields:  tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday,
 * tm_yday, and tm_isdst.
 */
lib7_val_t _lib7_Date_gmtime (lib7_state_t *lib7_state, lib7_val_t arg)
{
    time_t	t = (time_t)INT32_LIB7toC(arg);
    struct tm	*tm;

    tm = gmtime (&t);

    if (tm == NULL) return RAISE_SYSERR(lib7_state,0);

    LIB7_AllocWrite(lib7_state, 0, MAKE_DESC(DTAG_record, 9));
    LIB7_AllocWrite(lib7_state, 1, INT_CtoLib7(tm->tm_sec));
    LIB7_AllocWrite(lib7_state, 2, INT_CtoLib7(tm->tm_min));
    LIB7_AllocWrite(lib7_state, 3, INT_CtoLib7(tm->tm_hour));
    LIB7_AllocWrite(lib7_state, 4, INT_CtoLib7(tm->tm_mday));
    LIB7_AllocWrite(lib7_state, 5, INT_CtoLib7(tm->tm_mon));
    LIB7_AllocWrite(lib7_state, 6, INT_CtoLib7(tm->tm_year));
    LIB7_AllocWrite(lib7_state, 7, INT_CtoLib7(tm->tm_wday));
    LIB7_AllocWrite(lib7_state, 8, INT_CtoLib7(tm->tm_yday));
    LIB7_AllocWrite(lib7_state, 9, INT_CtoLib7(tm->tm_isdst));

    return LIB7_Alloc(lib7_state, 9);
}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
