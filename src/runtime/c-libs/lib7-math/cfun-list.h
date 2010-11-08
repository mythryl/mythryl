/* cfun-list.h
 *
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"Lib7-Math"
#define CLIB_VERSION	"1.1"
#define CLIB_DATE	"November 1, 1996"
#endif

CFUNC("ctlRoundingMode",	_lib7_Math_ctlrndmode,	"int option -> int")
CFUNC("cos64",			_lib7_Math_cos64,		"real -> real")
CFUNC("sin64",			_lib7_Math_sin64,		"real -> real")
CFUNC("exp64",			_lib7_Math_exp64,		"real -> (real * int)")
CFUNC("log64",			_lib7_Math_log64,		"real -> real")
CFUNC("sqrt64",			_lib7_Math_sqrt64,	"real -> real")
CFUNC("atan64", 		_lib7_Math_atan64,        "real -> real")


/* COPYRIGHT (c) 1996 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
