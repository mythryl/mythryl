/* strftime.c
 *
 */

#include "../../config.h"

#include <time.h>
#include <string.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_Date_strftime :
 *    (String, (Int, Int, Int, Int, Int, Int, Int, Int, Int)) -> String
 *
 * This takes a format field and nine integer fields (sec, min, hour, mday, mon,
 * year, wday, yday, and isdst), and converts it into a string representation
 * according to the format string.
 */
lib7_val_t _lib7_Date_strftime (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	fmt = REC_SEL(arg, 0);
    lib7_val_t	date;
    struct tm	tm;
    char	buf[512];
    size_t	size;

    date	= REC_SEL(arg, 1);
    tm.tm_sec	= REC_SELINT(date, 0);
    tm.tm_min	= REC_SELINT(date, 1);
    tm.tm_hour	= REC_SELINT(date, 2);
    tm.tm_mday	= REC_SELINT(date, 3);
    tm.tm_mon	= REC_SELINT(date, 4);
    tm.tm_year	= REC_SELINT(date, 5);
    tm.tm_wday	= REC_SELINT(date, 6);
    tm.tm_yday	= REC_SELINT(date, 7);
    tm.tm_isdst	= REC_SELINT(date, 8);

    size = strftime (buf, sizeof(buf), STR_LIB7toC(fmt), &tm);
    if (size > 0) {
        lib7_val_t result = LIB7_AllocString(lib7_state, size);
	strncpy (STR_LIB7toC(result), buf, size);
	return result;
    } else {
        return RAISE_ERROR(lib7_state, "strftime failed");
    }
}


/* COPYRIGHT (c) 1996 AT&T Research.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
