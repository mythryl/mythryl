// cfun-list.h
//
//
// This file lists the "time" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  get_time_of_day:  Void -> (int32::Int, Int)
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "time", fun_name => "timeofday" };
// 
// or such -- see   src/lib/std/src/time-guts.pkg
//
// We get #included by both:
//
//     src/c/lib/time/libmythryl-time.c
//     src/c/lib/time/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"time"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"December 16, 1994"
#endif

CFUNC("gettime","gettime",	_lib7_Time_gettime,		"")
CFUNC("timeofday","timeofday",	_lib7_Time_timeofday,		"Void -> (int32::Int, Int)")



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

