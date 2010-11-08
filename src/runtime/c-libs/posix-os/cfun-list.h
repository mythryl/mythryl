/* cfun-list.h
 *
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

/*
###              "I asked [Victor] Weisskopf how much
###               mathematics a physics student needs
###               to know, to which he answered with
###               a smile: `More.'"
###
###                                 -- Minhyong Kim
 */


#ifndef CLIB_NAME
#define CLIB_NAME	"POSIX-OS"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"December 21, 1995"
#endif

CFUNC("poll",		_lib7_OS_poll,		"((int * word) list * (int * int) option) -> (int * word) list")
CFUNC("tmpname",	_lib7_OS_tmpname,		"Void -> String")



/* COPYRIGHT (c) 1994 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
