/* liblib7-mp.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "c-library.h"
#include "cfun-proto-list.h"


/*/* The table of C functions and Lib7 names */
#define CFUNC(NAME, FUNC, LIB7TYPE)	CFUNC_BIND(NAME, FUNC, LIB7TYPE)
static cfunc_naming_t CFunTable[] = {
#include "cfun-list.h"
	CFUNC_NULL_BIND
    };
#undef CFUNC


/* the MP library */
c_library_t	    Lib7_MP_Library = {
	CLIB_NAME,
	CLIB_VERSION,
	CLIB_DATE,
	NULL,
	CFunTable
    };



/* COPYRIGHT (c) 1994 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
