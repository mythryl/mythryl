/* lib7-opencv-lib.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "c-library.h"
#include "cfun-proto-list.h"
#include "lib7-c.h"


/*/* The table of C functions and Lib7 names */
#define CFUNC(NAME, FUNC, LIB7TYPE)	CFUNC_BIND(NAME, FUNC, LIB7TYPE)
static cfunc_naming_t CFunTable[] = {
#include "cfun-list.h"
	CFUNC_NULL_BIND
    };
#undef CFUNC

#if !(HAVE_OPENCV_CV_H && HAVE_OPENCV_HIGHGUI_H && HAVE_LIBCV && HAVE_LIBCVAUX && HAVE_LIBHIGHGUI)
char* no_opencv_support_in_runtime = "No OpenCV support in runtime";
#endif

/* the opencv library */
c_library_t	    Lib7_OpenCV_Library = {
	CLIB_NAME,
	CLIB_VERSION,
	CLIB_DATE,
	NULL,
	CFunTable
    };


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
