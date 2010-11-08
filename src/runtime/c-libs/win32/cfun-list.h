/* cfun-list.h
 *
 *
 * utility win32 C functions
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"WIN32"
#define CLIB_VERSION	"0.1"
#define CLIB_DATE	"October 11, 1996"
#endif

CFUNC("get_const",	   _lib7_win32_get_const,	"String -> word32")
CFUNC("get_last_error",    _lib7_win32_get_last_error, "Void -> word32")
CFUNC("debug",             _lib7_win32_debug, "String -> Void")


/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
