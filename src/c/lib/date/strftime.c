// strftime.c

#include "../../mythryl-config.h"

#include <stdio.h>
#include <time.h>
#include <string.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

// One of the library bindings exported via
//     src/c/lib/date/cfun-list.h
// and thence
//     src/c/lib/date/libmythryl-date.c



Val   _lib7_Date_strftime   (Task* task,  Val arg) {
    //===================
    //
    // Mythryl type:    (String, (Int, Int, Int, Int, Int, Int, Int, Int, Int))  ->  String
    //
    // This takes a format field and nine integer fields (sec, min, hour, mday, mon,
    // year, wday, yday, and isdst), and converts it into a string representation
    // according to the format string.
    //
    // This fn gets bound to   strf_time   in:
    //
    //     src/lib/std/src/date.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Date_strftime");

    Val	fmt = GET_TUPLE_SLOT_AS_VAL(arg, 0);
    Val	date;
    struct tm	tm;
    char	buf[512];
    size_t	size;

    date	= GET_TUPLE_SLOT_AS_VAL(arg, 1);
    tm.tm_sec	= GET_TUPLE_SLOT_AS_INT(date, 0);
    tm.tm_min	= GET_TUPLE_SLOT_AS_INT(date, 1);
    tm.tm_hour	= GET_TUPLE_SLOT_AS_INT(date, 2);
    tm.tm_mday	= GET_TUPLE_SLOT_AS_INT(date, 3);
    tm.tm_mon	= GET_TUPLE_SLOT_AS_INT(date, 4);
    tm.tm_year	= GET_TUPLE_SLOT_AS_INT(date, 5);
    tm.tm_wday	= GET_TUPLE_SLOT_AS_INT(date, 6);
    tm.tm_yday	= GET_TUPLE_SLOT_AS_INT(date, 7);
    tm.tm_isdst	= GET_TUPLE_SLOT_AS_INT(date, 8);

    Mythryl_Heap_Value_Buffer fmt_buf;
    //
    {   void* c_fmt
	    =
	    buffer_mythryl_heap_value(
		//
		&fmt_buf,
		(void*) HEAP_STRING_AS_C_STRING(fmt),
		 strlen(HEAP_STRING_AS_C_STRING(fmt) ) +1		// '+1' for terminal NUL on string.
	     );
	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_Date_strftime", arg );
	    //
	    size = strftime (buf, sizeof(buf), c_fmt, &tm);							// This call might not be slow enough to need CEASE/BEGIN guards. (Cannot return EINTR.)
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_Date_strftime" );
	//
	unbuffer_mythryl_heap_value( &fmt_buf );    
    }

    if (size <= 0)   return RAISE_ERROR__MAY_HEAPCLEAN(task, "strftime failed", NULL);

    Val                               result = allocate_nonempty_ascii_string__may_heapclean(task, size, NULL);	// Tried 'size+1' for terminal NUL byte here:  It injected NULs into text logfiles. Ungood. -- 2011-11-19 CrT
    strncpy (HEAP_STRING_AS_C_STRING( result ), buf, size);
    return                            result;
}


// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

