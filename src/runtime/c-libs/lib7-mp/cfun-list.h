/* cfun-list.h
 *
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"Lib7-MP"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"December 18, 1994"
#endif

CFUNC("acquire_proc",	_lib7_MP_acquire_proc,	"")
CFUNC("max_procs",	_lib7_MP_max_procs,	"")
CFUNC("release_proc",	_lib7_MP_release_proc,	"")
CFUNC("spin_lock",	_lib7_MP_spin_lock,	"")



/* COPYRIGHT (c) 1994 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
