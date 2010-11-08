/* cfun-list.h
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"UNIX-Dynload"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"January 1, 2001"
#endif

CFUNC("dlopen",  _lib7_U_Dynload_dlopen,  "String option * Bool * Bool -> unt32.word")
CFUNC("dlsym",   _lib7_U_Dynload_dlsym,   "unt32.word * String -> unt32.word")
CFUNC("dlclose", _lib7_U_Dynload_dlclose, "unt32.word -> Void")
CFUNC("dlerror", _lib7_U_Dynload_dlerror, "Void -> String option")


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

