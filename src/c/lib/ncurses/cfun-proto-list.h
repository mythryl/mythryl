// cfun-proto-list.h
//
// External definitions for the C functions.
//
// This file gets #included (only) in:
//
//     src/c/lib/ncurses/libmythryl-ncurses.c

#ifndef _CFUN_PROTO_LIST_
#define _CFUN_PROTO_LIST_

#include "mythryl-callable-c-libraries.h"

#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_PROTO(NAME, FUNC, LIB7TYPE)
#include "cfun-list.h"
#undef CFUNC

#endif // _CFUN_PROTO_LIST_


// COPYRIGHT (c) 1194 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

