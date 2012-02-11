// asctime.c

#include "../../mythryl-config.h"

#include <stdio.h>
#include <time.h>
#include <string.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"

// One of the library bindings exported via
//     src/c/lib/date/cfun-list.h
// and thence
//     src/c/lib/date/libmythryl-date.c


#define DATE_LEN	24	// We discard the trailing \n\0


Val   _lib7_Date_ascii_time   (Task* task, Val arg)   {
    //=====================
    //
    // Mythryl type:  (Int, Int, Int, Int, Int, Int, Int, Int, Int) -> String
    //
    // This takes a nine-tuple date (fields sec, min, hour, mday, mon, year, wday,
    // yday, and isdst), and converts it into a string representation.
    //
    // This fn gets bound to 'ascii_time' in:
    //
    //     src/lib/std/src/date.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Date_ascii_time");

    struct tm	tm;
    //
    tm.tm_sec	= GET_TUPLE_SLOT_AS_INT(arg, 0);
    tm.tm_min	= GET_TUPLE_SLOT_AS_INT(arg, 1);
    tm.tm_hour	= GET_TUPLE_SLOT_AS_INT(arg, 2);
    tm.tm_mday	= GET_TUPLE_SLOT_AS_INT(arg, 3);
    tm.tm_mon	= GET_TUPLE_SLOT_AS_INT(arg, 4);
    tm.tm_year	= GET_TUPLE_SLOT_AS_INT(arg, 5);
    tm.tm_wday	= GET_TUPLE_SLOT_AS_INT(arg, 6);
    tm.tm_yday	= GET_TUPLE_SLOT_AS_INT(arg, 7);
    tm.tm_isdst	= GET_TUPLE_SLOT_AS_INT(arg, 8);

    Val result = allocate_nonempty_ascii_string__may_heapclean(task, DATE_LEN);

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_Date_ascii_time", arg );
	//
        char* string = asctime( &tm );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_Date_ascii_time" );

    strncpy (HEAP_STRING_AS_C_STRING(result), string, DATE_LEN);

    return result;
}


// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

