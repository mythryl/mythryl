// cfun-proto-list.h

#ifndef _CFUN_PROTO_LIST_
#define _CFUN_PROTO_LIST_

#include "mythryl-callable-c-libraries.h"

// External definitions for the "posix_error"
// library C functions.  This file gets #included by:
//
//     src/c/lib/posix-error/libmythryl-posix-error.c
//
#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_PROTO(NAME, FUNC, LIB7TYPE)
#include "cfun-list.h"								// Actual function list is in   src/c/lib/heap/cfun-list.h
#undef CFUNC

#endif // _CFUN_PROTO_LIST_


// COPYRIGHT (c) 1984 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.

