// gmtime.c

#include "../../config.h"

#include <time.h>
#include "runtime-base.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"
#include "lib7-c.h"

// One of the library bindings exported via
//     src/c/lib/date/cfun-list.h
// and thence
//     src/c/lib/date/libmythryl-date.c



Val   _lib7_Date_greanwich_mean_time   (Task* task,  Val arg) {
    //=================
    //
    // Mythryl type:  int32::Int -> (Int, Int, Int, Int, Int, Int, Int, Int, Int)
    //
    // Takes a UTC time value (in seconds), and converts it to a 9-tuple with
    // the fields:  tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday,
    // tm_yday, and tm_isdst.
    //
    // This fn gets bound to   gm_time'   in:
    //
    //     src/lib/std/src/date.pkg

    time_t t =  (time_t) INT32_LIB7toC(arg);

    struct tm* tm =  gmtime (&t);

    if (tm == NULL) return RAISE_SYSERR(task,0);

    LIB7_AllocWrite(task, 0, MAKE_TAGWORD(PAIRS_AND_RECORDS_BTAG, 9));
    LIB7_AllocWrite(task, 1, TAGGED_INT_FROM_C_INT(tm->tm_sec));
    LIB7_AllocWrite(task, 2, TAGGED_INT_FROM_C_INT(tm->tm_min));
    LIB7_AllocWrite(task, 3, TAGGED_INT_FROM_C_INT(tm->tm_hour));
    LIB7_AllocWrite(task, 4, TAGGED_INT_FROM_C_INT(tm->tm_mday));
    LIB7_AllocWrite(task, 5, TAGGED_INT_FROM_C_INT(tm->tm_mon));
    LIB7_AllocWrite(task, 6, TAGGED_INT_FROM_C_INT(tm->tm_year));
    LIB7_AllocWrite(task, 7, TAGGED_INT_FROM_C_INT(tm->tm_wday));
    LIB7_AllocWrite(task, 8, TAGGED_INT_FROM_C_INT(tm->tm_yday));
    LIB7_AllocWrite(task, 9, TAGGED_INT_FROM_C_INT(tm->tm_isdst));

    return LIB7_Alloc(task, 9);
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

