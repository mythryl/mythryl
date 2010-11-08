/* cfun-list.h
 *
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"POSIX-SysDB"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"February 16, 1995"
#endif

CFUNC("getgrgid",  _lib7_P_SysDB_getgrgid,  "word -> String * word * String list")
CFUNC("getgrnam",  _lib7_P_SysDB_getgrnam,  "String -> String * word * String list")
CFUNC("getpwuid",  _lib7_P_SysDB_getpwuid,  "word -> String * word * word * String * String")
CFUNC("getpwnam",  _lib7_P_SysDB_getpwnam,  "String -> String * word * word * String * String")



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
