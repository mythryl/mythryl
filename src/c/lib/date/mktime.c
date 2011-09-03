// mktime.c

#include "../../config.h"

#include <time.h>
#include "runtime-base.h"
#include "lib7-c.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"

// One of the library bindings exported via
//     src/c/lib/date/cfun-list.h
// and thence
//     src/c/lib/date/libmythryl-date.c



Val   _lib7_Date_make_time   (Task* task,  Val arg) {
    //=================
    //
    // Mythryl type:  (Int, Int, Int, Int, Int, Int, Int, Int, Int)   ->    int1::Int
    //
    // This takes a 9-tuple with the fields: tm_sec, tm_min, tm_hour, tm_mday,
    // tm_mon, tm_year, tm_wday, tm_yday, tm_isdst, and returns the corresponding
    // localtime value (in seconds).
    //
    // This fn gets bound to   make_time'   in:
    //
    //     src/lib/std/src/date.pkg

    struct tm	tm;
    time_t	t;

    tm.tm_sec	= GET_TUPLE_SLOT_AS_INT(arg, 0);
    tm.tm_min	= GET_TUPLE_SLOT_AS_INT(arg, 1);
    tm.tm_hour	= GET_TUPLE_SLOT_AS_INT(arg, 2);
    tm.tm_mday	= GET_TUPLE_SLOT_AS_INT(arg, 3);
    tm.tm_mon	= GET_TUPLE_SLOT_AS_INT(arg, 4);
    tm.tm_year	= GET_TUPLE_SLOT_AS_INT(arg, 5);
//  tm.tm_wday  = GET_TUPLE_SLOT_AS_INT(arg, 6);   // Ignored by mktime.
//  tm.tm_yday  = GET_TUPLE_SLOT_AS_INT(arg, 7);   // ignored by mktime.
    tm.tm_isdst	= GET_TUPLE_SLOT_AS_INT(arg, 8);

    t = mktime (&tm);

    if (t < 0) {
        //
        return RAISE_ERROR(task, "Invalid date");
    } else {

	Val result;

	INT1_ALLOC(task, result, t);

	return result;
    }
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

