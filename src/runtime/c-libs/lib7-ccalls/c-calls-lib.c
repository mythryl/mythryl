/* c-calls-lib.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "c-library.h"
#include "cfun-proto-list.h"


/*/* The table of C functions and Lib7 names */
#define C_CALLS_CFUNC(NAME, FUNC, CTYPE, CARGS)	\
            CFUNC_BIND(NAME, (cfunc_t) FUNC, "")
#define CFUNC(NAME, FUNC, LIB7TYPE)	CFUNC_BIND(NAME, FUNC, LIB7TYPE)
static cfunc_naming_t CFunTable[] = {
#include "cfun-list.h"
	CFUNC_NULL_BIND
    };
#undef CFUNC


/* the c-calls library */
c_library_t	    Lib7_CCalls_Library = {
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
