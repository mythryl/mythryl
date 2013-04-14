// cfun-list.h
//
//
// This file lists the "posix_os" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  tmpname:  Void -> String
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "posix_os", fun_name => "tmpname" };
// 
// or such -- src/lib/std/src/posix/winix-file.pkg
//
// We get #included by both:
//
//     src/c/lib/posix-os/libmythryl-posix-os.c
//     src/c/lib/posix-os/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c

/*
###              "I asked [Victor] Weisskopf how much
###               mathematics a physics student needs
###               to know, to which he answered with
###               a smile: `More.'"
###
###                                 -- Minhyong Kim
*/


#ifndef CLIB_NAME
#define CLIB_NAME	"posix_os"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"December 21, 1995"
#endif

CFUNC("select","poll",		_lib7_OS_select,	"(List((Int, Unt)), Null_Or((Int, Int))) -> List( (Int, Unt))")
CFUNC("tmpname","tmpname",	_lib7_OS_tmpname,	"Void -> String")



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

