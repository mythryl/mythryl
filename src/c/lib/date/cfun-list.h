// cfun-list.h
//
// This file lists the "date" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  ascii_time:  Tm -> String
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "date", fun_name => "ascii_time" };
// 
// or such -- src/lib/std/src/date.pkg
// It gets #included by both:
//
//     src/c/lib/date/libmythryl-date.c
//     src/c/lib/date/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"date"
#define CLIB_VERSION	"1.1"
#define CLIB_DATE	"September 5, 1996"
#endif

CFUNC("ascii_time","ascii_time",	_lib7_Date_ascii_time,		"")
CFUNC("local_time","local_time",	_lib7_Date_local_time,		"")
CFUNC("greenwich_mean_time",	"greenwich_mean_time",	_lib7_Date_greanwich_mean_time,	"")
CFUNC("make_time","make_time",		_lib7_Date_make_time,		"")
CFUNC("strftime","strftime",		_lib7_Date_strftime,		"")



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

