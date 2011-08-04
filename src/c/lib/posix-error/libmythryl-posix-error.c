// libmythryl-posix-error.c

#include "../../config.h"

#include "runtime-base.h"
#include "mythryl-callable-c-libraries.h"
#include "cfun-proto-list.h"


// This file defines the "posix_error" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  list_errors:  Void -> List(System_Constant)
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "posix_error", fun_name => "listerrors" };
// 
// or such -- see src/lib/std/src/posix-1003.1b/posix-error.pkg
//
#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_BIND(NAME, NAME2, FUNC, LIB7TYPE)
static Mythryl_Name_With_C_Function CFunTable[] = {
#include "cfun-list.h"											// Actual function list is in   src/c/lib/posix-error/cfun-list.h
	CFUNC_NULL_BIND
    };
#undef CFUNC


// The POSIX Error library.
//
// Our record                Libmythryl_Posix_Error
// gets compiled into        src/c/lib/mythryl-callable-c-libraries-list.h
// and thus ultimately       mythryl_callable_c_libraries_local []
// in                        src/c/lib/mythryl-callable-c-libraries.c
//
Mythryl_Callable_C_Library	    Libmythryl_Posix_Error = {						// Mythryl_Callable_C_Library		def in    src/c/h/mythryl-callable-c-libraries.h
    //                              ======================
    CLIB_NAME,
    CLIB_VERSION,
    CLIB_DATE,
    NULL,
    CFunTable
};



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

