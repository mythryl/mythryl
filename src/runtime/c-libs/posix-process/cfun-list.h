/* cfun-list.h
 *
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"POSIX-Process"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"February 16, 1995"
#endif

CFUNC("osval",  _lib7_P_Process_osval,    "String -> Int")
CFUNC("fork",   _lib7_P_Process_fork,     "Void -> Int")
CFUNC("exec",   _lib7_P_Process_exec,     "(String, List(String)) -> X")
CFUNC("exece",  _lib7_P_Process_exece,    "(String, List(String), List(String)) -> X")
CFUNC("execp",   _lib7_P_Process_execp,   "(String, List(String)) -> X")
CFUNC("waitpid", _lib7_P_Process_waitpid, "(Int, Unt) -> (Int, Int, Int)")
CFUNC("exit",    _lib7_P_Process_exit,    "Int -> X")
CFUNC("kill",    _lib7_P_Process_kill,    "(Int, Int) -> Void")
CFUNC("alarm",   _lib7_P_Process_alarm,   "Int -> Int")
CFUNC("pause",   _lib7_P_Process_pause,   "Void -> Void")
CFUNC("sleep",   _lib7_P_Process_sleep,   "Int -> Int")



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
