// libmythryl-signal.c

#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "mythryl-callable-c-libraries.h"
#include "cfun-proto-list.h"


// This file defines the "signal" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  get_signal_state':  Int -> Int
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "signal", fun_name => "get_signal_state" };
// 
// or such -- see   src/lib/std/src/nj/interprocess-signals-guts.pkg
//
#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_BIND(NAME, NAME2, FUNC, LIB7TYPE)
static Mythryl_Name_With_C_Function CFunTable[] = {
#include "cfun-list.h"											// Actual function list is in   src/c/lib/heap/cfun-list.h
	CFUNC_NULL_BIND
    };
#undef CFUNC


// The Signals library:
//
// Our record                Libmythryl_Signal
// gets compiled into        src/c/lib/mythryl-callable-c-libraries-list.h
// and thus ultimately       mythryl_callable_c_libraries__local []
// in                        src/c/lib/mythryl-callable-c-libraries.c
//
Mythryl_Callable_C_Library	    Libmythryl_Signal = {						// Mythryl_Callable_C_Library		def in    src/c/h/mythryl-callable-c-libraries.h
    //                              =================
    CLIB_NAME,
    CLIB_VERSION,
    CLIB_DATE,
    NULL,
    CFunTable
};





// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

