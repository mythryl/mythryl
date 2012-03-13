// cfun-list.h
//
//
// This file lists the "math" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  atan64:  Float -> Float
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "math", fun_name => "atan64" };
// 
// or such -- although it appears no functions currently do so (?!) with the exception of
//
//     ci::find_c_function { lib_name => "math", fun_name => "get_or_set_rounding_mode" };
// in
//     src/lib/std/src/ieee-float.pkg
//
// This file gets #included by both:
//
//     src/c/lib/math/libmythryl-math.c
//     src/c/lib/math/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c



#ifndef CLIB_NAME
#define CLIB_NAME	"math"
#define CLIB_VERSION	"1.1"
#define CLIB_DATE	"November 1, 1996"
#endif

CFUNC("get_or_set_rounding_mode","get_or_set_rounding_mode",	_lib7_Math_get_or_set_rounding_mode,	"Null_Or(Int) -> Int")
CFUNC("cos64","cos64",			_lib7_Math_cos64,	"Float -> Float")
CFUNC("sin64","sin64",			_lib7_Math_sin64,	"Float -> Float")
CFUNC("exp64","exp64",			_lib7_Math_exp64,	"Float -> (Flat, Int)")
CFUNC("log64","log64",			_lib7_Math_log64,	"Float -> Float")
CFUNC("sqrt64","sqrt64",		_lib7_Math_sqrt64,	"Float -> Float")
CFUNC("atan64","atan64", 		_lib7_Math_atan64,      "Float -> Float")


// COPYRIGHT (c) 1996 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

