// libmythryl-win32-io.c

#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "mythryl-callable-c-libraries.h"
#include "cfun-proto-list.h"


// The table of C functions and Mythryl names:
//
#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_BIND(NAME, NAME2, FUNC, LIB7TYPE)
static Mythryl_Name_With_C_Function CFunTable[] = {
#include "cfun-list.h"											// Actual function list is in   src/c/lib/win32-io/cfun-list.h
	CFUNC_NULL_BIND
    };
#undef CFUNC


// The win32 I/O library.
//
// Our record                Libmythryl_Win32_Io
// gets compiled into        src/c/lib/mythryl-callable-c-libraries-list.h
// and thus ultimately       mythryl_callable_c_libraries__local []
// in                        src/c/lib/mythryl-callable-c-libraries.c
//
Mythryl_Callable_C_Library	    Libmythryl_Win32_Io = {						// Mythryl_Callable_C_Library		def in    src/c/h/mythryl-callable-c-libraries.h
    //                              ================
    CLIB_NAME,
    CLIB_VERSION,
    CLIB_DATE,
    NULL,
    CFunTable
};



// COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

