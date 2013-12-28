// cfun-proto-list.h


#ifndef _CFUN_PROTO_LIST_
#define _CFUN_PROTO_LIST_

#include "mythryl-callable-c-libraries.h"

// External definitions for the "signal"
// library C functions.  This file gets #included by:
//
//     src/c/lib/signal/libmythryl-signal.c
//
#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_PROTO(NAME, FUNC, LIB7TYPE)
#include "cfun-list.h"								// Actual function list is in   src/c/lib/signal/cfun-list.h
#undef CFUNC

#endif // _CFUN_PROTO_LIST_


// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

