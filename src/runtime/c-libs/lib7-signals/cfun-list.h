/* cfun-list.h
 *
 *
 * This file lists the signals library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"Lib7-Signals"
#define CLIB_VERSION	"1.1"
#define CLIB_DATE	"October 29, 1995"
#endif

CFUNC("listSignals",	_lib7_Sig_listsigs,	"Void -> sysconst list")
CFUNC("getSigState",	_lib7_Sig_getsigstate,	"sysconst -> int")
CFUNC("setSigState",	_lib7_Sig_setsigstate,	"(sysconst * int) -> int")
CFUNC("getSigMask",	_lib7_Sig_getsigmask,	"Void -> sysconst list option")
CFUNC("setSigMask",	_lib7_Sig_setsigmask,	"sysconst list option -> Void")
CFUNC("pause",		_lib7_Sig_pause,		"Void -> Void")



/* COPYRIGHT (c) 1994 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
