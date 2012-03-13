// cfun-list.h
//
// This file lists the "ccalls" library of Mythryl-callable
// C functions, accessible at the Mythryl level via.  This appears
// referenced mainly by the (currently unmaintained) code in
//
//     src/lib/c-glue-old/ccalls.pkg 
//
// via code like
//
//	my CFnDatumMLtoC:  arg_desc * cdata -> (caddr * List( caddr ) )
//            = 
//	      mythryl_callable_c_library_interface::find_c_function { lib_name => "ccalls", fun_name => "convert_mythryl_value_to_c" };
// 
// We get #included by both:
//
//     src/c/lib/ccalls/libmythryl-ccalls.c
//     src/c/lib/ccalls/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"ccalls"
#define CLIB_VERSION	"0.0"
#define CLIB_DATE	"March 3, 1995"
#endif

CFUNC("ccall","ccall",		lib7_c_call,			"")
CFUNC("convert_mythryl_value_to_c","convert_mythryl_value_to_c",	lib7_convert_mythryl_value_to_c,		"")
CFUNC("convert_c_value_to_mythryl","convert_c_value_to_mythryl",	lib7_datumCtoML,		"")

#include "cutil-cfuns.h"



/* COPYRIGHT (c) 1994 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
 * released under Gnu Public Licence version 3.
 */

