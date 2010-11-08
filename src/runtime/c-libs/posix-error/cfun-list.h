/* cfun-list.h
 *
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"POSIX-Error"
#define CLIB_VERSION	"1.1"
#define CLIB_DATE	"December 31, 1996"
#endif

CFUNC("errmsg",		_lib7_P_Error_errmsg,		"word -> String")
CFUNC("geterror",	_lib7_P_Error_geterror,		"word -> sys_const")
CFUNC("listerrors",	_lib7_P_Error_listerrors,	"Void -> sys_const list")



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
