// cfun-proto-list.h


#ifndef _CFUN_PROTO_LIST_
#define _CFUN_PROTO_LIST_

#include "mythryl-callable-c-libraries.h"

// The external definitions for the C functions:
//
#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_PROTO(NAME, FUNC, LIB7TYPE)
#include "cfun-list.h"
#undef CFUNC

#endif /* !_CFUN_PROTO_LIST_ */


// COPYRIGHT (c) 1194 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

