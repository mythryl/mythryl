// cfun-proto-list.h

#ifndef _CFUN_PROTO_LIST_
#define _CFUN_PROTO_LIST_

#include "mythryl-callable-c-libraries.h"


// External definitions for the "ccalls"
// library C functions.  This file gets #included by:
//
//     src/c/lib/ccalls/libmythryl-ccalls.c
//

#define C_CALLS_CFUNC_PROTO(NAME, FUNC, CTYPE, CARGS)	\
	extern CTYPE FUNC CARGS; 

#define C_CALLS_CFUNC(NAME, NAME2, FUNC, CTYPE, CARGS)	\
           C_CALLS_CFUNC_PROTO(NAME,FUNC,CTYPE,CARGS)

#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_PROTO(NAME, FUNC, LIB7TYPE)
#include "cfun-list.h"								// Actual function list is in src/c/lib/ccalls/cfun-list.h
#undef CFUNC
#undef C_CALLS_CFUNC

#endif /* !_CFUN_PROTO_LIST_ */


// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

