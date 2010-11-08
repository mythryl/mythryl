/* cfun-proto-list.h
 *
 */

#ifndef _CFUN_PROTO_LIST_
#define _CFUN_PROTO_LIST_

#ifndef _C_LIBRARY_
#  include "c-library.h"
#endif


#define C_CALLS_CFUNC_PROTO(NAME, FUNC, CTYPE, CARGS)	\
	extern CTYPE FUNC CARGS; 

/* the external definitions for the C functions */
#define C_CALLS_CFUNC(NAME, FUNC, CTYPE, CARGS)    \
           C_CALLS_CFUNC_PROTO(NAME,FUNC,CTYPE,CARGS)
#define CFUNC(NAME, FUNC, LIB7TYPE)	CFUNC_PROTO(NAME, FUNC, LIB7TYPE)
#include "cfun-list.h"
#undef CFUNC
#undef C_CALLS_CFUNC

#endif /* !_CFUN_PROTO_LIST_ */


/* COPYRIGHT (c) 1994 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
