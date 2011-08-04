// cfun-list.h
//
// This file lists the "posix_error" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  list_errors:  Void -> List(System_Constant)
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "posix_error", fun_name => "listerrors" };
// 
// or such -- see src/lib/std/src/posix-1003.1b/posix-error.pkg
//
// We get #included by both:
//
//     src/c/lib/posix-error/libmythryl-posix-error.c
//     src/c/lib/posix-error/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c



#ifndef CLIB_NAME
#define CLIB_NAME	"posix_error"
#define CLIB_VERSION	"1.1"
#define CLIB_DATE	"December 31, 1996"
#endif

CFUNC("errmsg","errmsg",		_lib7_P_Error_errmsg,		"Int -> String")
CFUNC("geterror","geterror",	_lib7_P_Error_geterror,		"Int -> System_Constant")
CFUNC("listerrors","listerrors",	_lib7_P_Error_listerrors,	"Void -> List(Sys_Constant)")



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

