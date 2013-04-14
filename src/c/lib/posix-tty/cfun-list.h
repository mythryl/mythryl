// cfun-list.h
//
//
// This file lists the "posix_tty" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  tcdrain:  Int -> Void
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "posix_tty", fun_name => "tcdrain" };
// 
// or such -- see src/lib/std/src/psx/posix-tty.pkg
//
// We get #included by both:
//
//     src/c/lib/posix-tty/libmythryl-posix-tty.c
//     src/c/lib/posix-tty/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"posix_tty"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"August 22, 1995"
#endif

CFUNC("osval","osval",	     _lib7_P_TTY_osval,          "String -> Int")
CFUNC("tcgetattr","tcgetattr",   _lib7_P_TTY_tcgetattr,      "Int -> Termio_Rep")
CFUNC("tcsetattr","tcsetattr",   _lib7_P_TTY_tcsetattr,      "(Int, Int, Termio_Rep) -> Void")
CFUNC("tcsendbreak","tcsendbreak", _lib7_P_TTY_tcsendbreak,    "(Int, Int) -> Void")
CFUNC("tcdrain","tcdrain",     _lib7_P_TTY_tcdrain,        "Int -> Void")
CFUNC("tcflush","tcflush",     _lib7_P_TTY_tcflush,        "(Int, Int) -> Void")
CFUNC("tcflow","tcflow",      _lib7_P_TTY_tcflow,         "(Int, Int) -> Void")
CFUNC("tcgetpgrp","tcgetpgrp",   _lib7_P_TTY_tcgetpgrp,      "Int -> Int")
CFUNC("tcsetpgrp","tcsetpgrp",   _lib7_P_TTY_tcsetpgrp,      "(Int, Int) -> Void")



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

