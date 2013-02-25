// libmythryl-opencv.c

#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "mythryl-callable-c-libraries.h"
#include "cfun-proto-list.h"
#include "raise-error.h"


// The table of C functions and Lib7 names:
//
#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_BIND(NAME, NAME2, FUNC, LIB7TYPE)
static Mythryl_Name_With_C_Function CFunTable[] = {
#include "cfun-list.h"											// Actual function list is in src/c/lib/opencv/cfun-list.h
	CFUNC_NULL_BIND
    };
#undef CFUNC

#if !(HAVE_OPENCV_CV_H && HAVE_OPENCV_HIGHGUI_H && HAVE_LIBCV && HAVE_LIBCVAUX && HAVE_LIBHIGHGUI)
char* no_opencv_support_in_runtime = "No OpenCV support in runtime";
#endif

// The OpenCV library.
//
// Our record                Libmythryl_Opencv
// gets compiled into        src/c/lib/mythryl-callable-c-libraries-list.h
// and thus ultimately       mythryl_callable_c_libraries__local []
// in                        src/c/lib/mythryl-callable-c-libraries.c
//
Mythryl_Callable_C_Library	    Libmythryl_Opencvxxx = {						// Mythryl_Callable_C_Library		def in    src/c/h/mythryl-callable-c-libraries.h
    //                              =================
    CLIB_NAME,
    CLIB_VERSION,
    CLIB_DATE,
    NULL,
    CFunTable
};


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

