// cfun-list.h
//
//
// This file lists the "posix_process_environment" ("process environment") library
// of Mythryl-callable C functions, accessible at the Mythryl level via:
//
//     my  get_process_id:  Void -> Sy_Int
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "posix_process_environment", fun_name => "getpid" };
// 
// or such -- see src/lib/std/src/psx/posix-id.pkg
// 
// We get #included by both:
//
//     src/c/lib/posix-process-environment/libmythryl-posix-process-environment.c
//     src/c/lib/posix-process-environment/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"posix_process_environment"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"February 16, 1995"
#endif

CFUNC("getpid","getpid",    _lib7_P_ProcEnv_getpid,    "Void -> Int")				// Gets bound as get_process_id  in   src/lib/std/src/psx/posix-id.pkg
CFUNC("getppid","getppid",   _lib7_P_ProcEnv_getppid,   "Void -> Int")
CFUNC("getuid","getuid",    _lib7_P_ProcEnv_getuid,    "Void -> Unt")
CFUNC("geteuid","geteuid",   _lib7_P_ProcEnv_geteuid,   "Void -> Unt")
CFUNC("getgid","getgid",    _lib7_P_ProcEnv_getgid,    "Void -> Unt")
CFUNC("getegid","getegid",   _lib7_P_ProcEnv_getegid,   "Void -> Unt")
CFUNC("setuid","setuid",    _lib7_P_ProcEnv_setuid,    "word -> Void")
CFUNC("setgid","setgid",    _lib7_P_ProcEnv_setgid,    "word -> Void")
CFUNC("getgroups","getgroups", _lib7_P_ProcEnv_getgroups, "Void -> List(Unt)")
CFUNC("getlogin","getlogin",  _lib7_P_ProcEnv_getlogin,  "Void -> String")
CFUNC("getpgrp","getpgrp",   _lib7_P_ProcEnv_getpgrp,   "Void -> Int")
CFUNC("setsid","setsid",    _lib7_P_ProcEnv_setsid,    "Void -> Int")
CFUNC("setpgid","setpgid",   _lib7_P_ProcEnv_setpgid,   "(Int, Int) -> Void")
CFUNC("uname","uname",     _lib7_P_ProcEnv_uname,     "Void -> List (String, String)")
CFUNC("sysconf","sysconf",   _lib7_P_ProcEnv_sysconf,   "String -> Unt")
CFUNC("time","time",      _lib7_P_ProcEnv_time,      "Void -> Int")
CFUNC("times","times",     _lib7_P_ProcEnv_times,     "Void -> (Int, Int, Int, Int, Int)")
CFUNC("getenv","getenv",    _lib7_P_ProcEnv_getenv,    "String -> Null_Or(String)")
CFUNC("environ","environ",   _lib7_P_ProcEnv_environ,   "Void -> List(String)")
CFUNC("ctermid","ctermid",   _lib7_P_ProcEnv_ctermid,   "Void -> String")
CFUNC("ttyname","ttyname",   _lib7_P_ProcEnv_ttyname,   "Int -> String")
CFUNC("isatty","isatty",    _lib7_P_ProcEnv_isatty,    "Int -> Bool")



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

