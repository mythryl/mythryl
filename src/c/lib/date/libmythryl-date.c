// libmythryl-date.c

#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "mythryl-callable-c-libraries.h"
#include "cfun-proto-list.h"


// This file defines the "date" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  ascii_time:  Tm -> String
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "date", fun_name => "ascii_time" };
// 
// or such -- src/lib/std/src/date.pkg
//
#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_BIND(NAME, NAME2, FUNC, LIB7TYPE)
static Mythryl_Name_With_C_Function CFunTable[] = {
#include "cfun-list.h"										// Actual function list is in src/c/lib/date/cfun-list.h
	CFUNC_NULL_BIND
    };
#undef CFUNC


// the Date library:
//
// Our record                Libmythryl_Date
// gets compiled into        src/c/lib/mythryl-callable-c-libraries-list.h
// and thus ultimately       mythryl_callable_c_libraries__local []
// in                        src/c/lib/mythryl-callable-c-libraries.c
//
Mythryl_Callable_C_Library	    Libmythryl_Date = {						// Mythryl_Callable_C_Library		def in    src/c/h/mythryl-callable-c-libraries.h
    //                              ===============
    CLIB_NAME,
    CLIB_VERSION,
    CLIB_DATE,
    NULL,
    CFunTable
};


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

