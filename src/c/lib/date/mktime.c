// mktime.c

#include "../../mythryl-config.h"

#include <stdio.h>
#include <time.h>
#include "runtime-base.h"
#include "raise-error.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"

// One of the library bindings exported via
//     src/c/lib/date/cfun-list.h
// and thence
//     src/c/lib/date/libmythryl-date.c



Val   _lib7_Date_make_time   (Task* task,  Val arg) {
    //====================
    //
    // Mythryl type:  (Int, Int, Int, Int, Int, Int, Int, Int, Int)   ->    one_word_int::Int
    //
    // This takes a 9-tuple with the fields: tm_sec, tm_min, tm_hour, tm_mday,
    // tm_mon, tm_year, tm_wday, tm_yday, tm_isdst, and returns the corresponding
    // localtime value (in seconds).
    //
    // This fn gets bound to   make_time'   in:
    //
    //     src/lib/std/src/date.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Date_make_time");

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

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_Date_make_time", NULL );
	//
        t = mktime (&tm);
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_Date_make_time" );

    if (t < 0) {
        //
        return RAISE_ERROR__MAY_HEAPCLEAN(task, "Invalid date", NULL);
    } else {

	return  make_one_word_int(task,  t  );
    }
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

