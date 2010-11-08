/* cfun-list.h
 *
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"POSIX-ProcEnv"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"February 16, 1995"
#endif

CFUNC("getpid",    _lib7_P_ProcEnv_getpid,    "Void -> int")
CFUNC("getppid",   _lib7_P_ProcEnv_getppid,   "Void -> int")
CFUNC("getuid",    _lib7_P_ProcEnv_getuid,    "Void -> word")
CFUNC("geteuid",   _lib7_P_ProcEnv_geteuid,   "Void -> word")
CFUNC("getgid",    _lib7_P_ProcEnv_getgid,    "Void -> word")
CFUNC("getegid",   _lib7_P_ProcEnv_getegid,   "Void -> word")
CFUNC("setuid",    _lib7_P_ProcEnv_setuid,    "word -> Void")
CFUNC("setgid",    _lib7_P_ProcEnv_setgid,    "word -> Void")
CFUNC("getgroups", _lib7_P_ProcEnv_getgroups, "Void -> word list")
CFUNC("getlogin",  _lib7_P_ProcEnv_getlogin,  "Void -> String")
CFUNC("getpgrp",   _lib7_P_ProcEnv_getpgrp,   "Void -> int")
CFUNC("setsid",    _lib7_P_ProcEnv_setsid,    "Void -> int")
CFUNC("setpgid",   _lib7_P_ProcEnv_setpgid,   "int * int -> Void")
CFUNC("uname",     _lib7_P_ProcEnv_uname,     "Void -> (String * String) list")
CFUNC("sysconf",   _lib7_P_ProcEnv_sysconf,   "String -> word")
CFUNC("time",      _lib7_P_ProcEnv_time,      "Void -> int")
CFUNC("times",     _lib7_P_ProcEnv_times,     "Void -> int * int * int * int * int")
CFUNC("getenv",    _lib7_P_ProcEnv_getenv,    "String -> String option")
CFUNC("environ",   _lib7_P_ProcEnv_environ,   "Void -> String list")
CFUNC("ctermid",   _lib7_P_ProcEnv_ctermid,   "Void -> String")
CFUNC("ttyname",   _lib7_P_ProcEnv_ttyname,   "int -> String")
CFUNC("isatty",    _lib7_P_ProcEnv_isatty,    "int -> Bool")



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
