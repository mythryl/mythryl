/* cfun-list.h
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"Lib7-CCalls"
#define CLIB_VERSION	"0.0"
#define CLIB_DATE	"March 3, 1995"
#endif

CFUNC("c_call",		lib7_c_call,		"")
CFUNC("datumMLtoC",	lib7_datumMLtoC,		"")
CFUNC("datumCtoLib7",	lib7_datumCtoML,		"")

#include "cutil-cfuns.h"



/* COPYRIGHT (c) 1994 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

