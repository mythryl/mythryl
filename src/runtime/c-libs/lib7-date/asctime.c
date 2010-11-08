/* asctime.c
 *
 */

#include "../../config.h"

#include <time.h>
#include <string.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"

#define DATE_LEN	24	/* we discard the trailing \n\0 */


/* _lib7_Date_asctime : (Int, Int, Int, Int, Int, Int, Int, Int, Int) -> String
 *
 * This takes a nine-tuple date (fields sec, min, hour, mday, mon, year, wday,
 * yday, and isdst), and converts it into a string representation.
 */
lib7_val_t _lib7_Date_asctime (lib7_state_t *lib7_state, lib7_val_t arg)
{
    struct tm	tm;

    tm.tm_sec	= REC_SELINT(arg, 0);
    tm.tm_min	= REC_SELINT(arg, 1);
    tm.tm_hour	= REC_SELINT(arg, 2);
    tm.tm_mday	= REC_SELINT(arg, 3);
    tm.tm_mon	= REC_SELINT(arg, 4);
    tm.tm_year	= REC_SELINT(arg, 5);
    tm.tm_wday	= REC_SELINT(arg, 6);
    tm.tm_yday	= REC_SELINT(arg, 7);
    tm.tm_isdst	= REC_SELINT(arg, 8);

    {   lib7_val_t result = LIB7_AllocString(lib7_state, DATE_LEN);
	strncpy (STR_LIB7toC(result), asctime(&tm), DATE_LEN);
	return result;
    }
}


/* COPYRIGHT (c) 1996 AT&T Research.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
