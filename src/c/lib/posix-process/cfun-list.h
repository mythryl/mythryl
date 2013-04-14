// cfun-list.h
//
//
// This file lists the "posix_process" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  sleep':  Int -> Int
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "posix_process", fun_name => "sleep" };
// 
// or such -- see src/lib/std/src/psx/posix-process.pkg
// 
// We get #included by both:
//
//     src/c/lib/posix-process/libmythryl-posix-process.c
//     src/c/lib/posix-process/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"posix_process"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"February 16, 1995"
#endif

CFUNC("osval","osval",  _lib7_P_Process_osval,    "String -> Int")
CFUNC("fork","fork",   _lib7_P_Process_fork,     "Void -> Int")
CFUNC("exec","exec",   _lib7_P_Process_exec,     "(String, List(String)) -> X")
CFUNC("exece","exece",  _lib7_P_Process_exece,    "(String, List(String), List(String)) -> X")
CFUNC("execp","execp",   _lib7_P_Process_execp,   "(String, List(String)) -> X")
CFUNC("waitpid","waitpid", _lib7_P_Process_waitpid, "(Int, Unt) -> (Int, Int, Int)")
CFUNC("exit","exit",    _lib7_P_Process_exit,    "Int -> X")
CFUNC("kill","kill",    _lib7_P_Process_kill,    "(Int, Int) -> Void")
CFUNC("alarm","alarm",   _lib7_P_Process_alarm,   "Int -> Int")
CFUNC("pause","pause",   _lib7_P_Process_pause,   "Void -> Void")
CFUNC("sleep","sleep",   _lib7_P_Process_sleep,   "Int -> Int")



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

