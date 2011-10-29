// libmythryl-ccalls.c

#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "mythryl-callable-c-libraries.h"
#include "cfun-proto-list.h"


// This file defines the "ccalls" library of Mythryl-callable
// C functions for calling C from Mythryl.  This appears referenced
// mainly by the (currently unmaintained) code in
//
//     src/lib/c-glue-old/ccalls.pkg 
//
// via code like
//
//	my CFnDatumMLtoC:  arg_desc * cdata -> (caddr * List( caddr ) )
//            = 
//	      mythryl_callable_c_library_interface::find_c_function { lib_name => "ccalls", fun_name => "convert_mythryl_value_to_c" };
//


#define C_CALLS_CFUNC(NAME, NAME2, FUNC, CTYPE, CARGS)	    CFUNC_BIND(NAME, NAME2, (Mythryl_Callable_C_Function) FUNC, "")

#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_BIND(NAME, NAME2, FUNC, LIB7TYPE)

static Mythryl_Name_With_C_Function CFunTable[] = {
#include "cfun-list.h"											// Actual function list is in src/c/lib/ccalls/cfun-list.h
	CFUNC_NULL_BIND
    };
#undef CFUNC


// The "ccalls" library.
//
// Our record                Libmythryl_Ccalls
// gets compiled into        src/c/lib/mythryl-callable-c-libraries-list.h
// and thus ultimately       mythryl_callable_c_libraries_local []
// in                        src/c/lib/mythryl-callable-c-libraries.c
//
Mythryl_Callable_C_Library	    Libmythryl_Ccalls = {						// Mythryl_Callable_C_Library		def in    src/c/h/mythryl-callable-c-libraries.h
    //                              ===================
    CLIB_NAME,
    CLIB_VERSION,
    CLIB_DATE,
    NULL,
    CFunTable
};


// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

