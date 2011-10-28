// libmythryl-multicore.c
//
// Here we export to
//
//     src/lib/std/src/posix-thread.api
//     src/lib/std/src/posix-thread.pkg
//
// some of the functionality defined in
//
//     src/c/h/runtime-multicore.h
//
// and implemented in the platform-specific files
//
//     src/c/pthread/sgi-multicore.c
//     src/c/pthread/solaris-multicore.c


#include "../../config.h"

#include "runtime-base.h"
#include "mythryl-callable-c-libraries.h"
#include "cfun-proto-list.h"


// This file defines the "multicore" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my acquire_pthread:   (Thread, Fate) -> Bool
//         =
//         mythryl_callable_c_library_interface::find_c_function  { lib_name => "multicore", fun_name => "acquire_pthread" };
// 
// or such.

#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_BIND(NAME, NAME2, FUNC, LIB7TYPE)
static Mythryl_Name_With_C_Function CFunTable[] = {
#include "cfun-list.h"											// Actual function list is in src/c/lib/pthread/cfun-list.h
	CFUNC_NULL_BIND
    };
#undef CFUNC


// The multicore ("multiprocessing") library.
//
// Our record                Libmythryl_Multicore
// gets compiled into        src/c/lib/mythryl-callable-c-libraries-list.h
// and thus ultimately       mythryl_callable_c_libraries_local []
// in                        src/c/lib/c-libraries.c
//
Mythryl_Callable_C_Library	    Libmythryl_Multicore = {						// Mythryl_Callable_C_Library		def in    src/c/h/mythryl-callable-c-libraries.h
    //                              ====================
    CLIB_NAME,
    CLIB_VERSION,
    CLIB_DATE,
    NULL,
    CFunTable
};



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

