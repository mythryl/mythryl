/* cfun-list.h
 *
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"Lib7-Prof"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"December 15, 1994"
#endif

CFUNC("setTimer",	_lib7_Prof_setptimer,	"Bool -> Void")
CFUNC("getQuantum",	_lib7_Prof_getpquantum,	"Void -> int")
CFUNC("setTimeArray",	_lib7_Prof_setpref,	"word Rw_Vector option -> Void")


/* COPYRIGHT (c) 1994 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
