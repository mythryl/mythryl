/* cfun-list.h
 *
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"POSIX-TTY"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"August 22, 1995"
#endif

CFUNC("osval",	     _lib7_P_TTY_osval,          "String -> int")
CFUNC("tcgetattr",   _lib7_P_TTY_tcgetattr,      "int -> termio_rep")
CFUNC("tcsetattr",   _lib7_P_TTY_tcsetattr,      "int * int * termio_rep -> Void")
CFUNC("tcsendbreak", _lib7_P_TTY_tcsendbreak,    "int * int -> Void")
CFUNC("tcdrain",     _lib7_P_TTY_tcdrain,        "int -> Void")
CFUNC("tcflush",     _lib7_P_TTY_tcflush,        "int * int -> Void")
CFUNC("tcflow",      _lib7_P_TTY_tcflow,         "int * int -> Void")
CFUNC("tcgetpgrp",   _lib7_P_TTY_tcgetpgrp,      "int -> int")
CFUNC("tcsetpgrp",   _lib7_P_TTY_tcsetpgrp,      "int * int -> Void")



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
