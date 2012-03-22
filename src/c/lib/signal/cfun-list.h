// cfun-list.h
//
// This file lists the "signal" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  list_signals':  Void -> String
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "signal", fun_name => "listSignals" };
// 
// or such -- see   src/lib/std/src/nj/runtime-signals-guts.pkg
// 
// We get #included by both:
//
//     src/c/lib/signal/libmythryl-signal.c
//     src/c/lib/signal/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"signal"
#define CLIB_VERSION	"1.1"
#define CLIB_DATE	"October 29, 1995"
#endif

CFUNC("listSignals","listSignals",	_lib7_Sig_listsigs,	"Void -> List(System_Constant)")
CFUNC("getSigState","getSigState",	_lib7_Sig_getsigstate,	"System_Constant -> Int")
CFUNC("setSigState","setSigState",	_lib7_Sig_setsigstate,	"(System_Constant, Int) -> Int")
CFUNC("getSigMask","getSigMask",	_lib7_Sig_getsigmask,	"Void -> Null_Or(List(System_Constant))")
CFUNC("setSigMask","setSigMask",	_lib7_Sig_setsigmask,	"Null_Or(List(System_Constant)) -> Void")
CFUNC("pause","pause",			_lib7_Sig_pause,	"Void -> Void")



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

