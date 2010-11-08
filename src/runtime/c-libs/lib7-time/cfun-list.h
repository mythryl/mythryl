/* cfun-list.h
 *
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"Lib7-Time"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"December 16, 1994"
#endif

CFUNC("gettime",	_lib7_Time_gettime,		"")
CFUNC("timeofday",	_lib7_Time_timeofday,		"")



/* COPYRIGHT (c) 1994 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
