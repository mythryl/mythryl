// cfun-list.h
//
// This file lists the "posix_signal" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  osval:  String -> Int
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "posix_signal", fun_name => "osval" };
// 
// or such -- src/lib/std/src/posix-1003.1b/posix-signal.pkg
//
// We get #included by both:
//
//     src/c/lib/posix-signal/libmythryl-posix-signal.c
//     src/c/lib/posix-signal/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"posix_signal"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"February 16, 1995"
#endif

CFUNC("osval","osval",	_lib7_P_Signal_osval,		"String -> Int")



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

