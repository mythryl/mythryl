/* cfun-list.h
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"Lib7-Date"
#define CLIB_VERSION	"1.1"
#define CLIB_DATE	"September 5, 1996"
#endif

CFUNC("ascTime",	_lib7_Date_asctime,		"")
CFUNC("localTime",	_lib7_Date_localtime,		"")
CFUNC("gmTime",		_lib7_Date_gmtime,		"")
CFUNC("mkTime",		_lib7_Date_mktime,		"")
CFUNC("strfTime",	_lib7_Date_strftime,		"")



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 *
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
