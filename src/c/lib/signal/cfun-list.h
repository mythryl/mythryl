// cfun-list.h
//
// This file lists the "signal" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  list_signals':  Void -> String
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "signal", fun_name => "listSignals" };
// 
// or such -- see   src/lib/std/src/nj/interprocess-signals-guts.pkg
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

CFUNC("set_signal_mask","set_signal_mask",							_lib7_Sig_setsigmask,					"Null_Or(List(Int)) -> Void")
CFUNC("pause","pause",										_lib7_Sig_pause,					"Void -> Void")
CFUNC("get_signal_mask","get_signal_mask",							_lib7_Sig_get_signal_mask,				"Void -> Null_Or(List(Int))")
CFUNC("get_signal_state","get_signal_state",							_lib7_Sig_get_signal_state,				"Int -> Int")
CFUNC("set_signal_state","set_signal_state",							_lib7_Sig_set_signal_state,				"(Int, Int) -> Int")
CFUNC("signal_is_supported_by_host_os","signal_is_supported_by_host_os",			_lib7_Sig_signal_is_supported_by_host_os,		"Int -> Bool")
CFUNC("ascii_signal_name_to_portable_signal_id","ascii_signal_name_to_portable_signal_id",	_lib7_Sig_ascii_signal_name_to_portable_signal_id,	"String -> Int")
CFUNC("maximum_valid_portable_signal_id","maximum_valid_portable_signal_id",			_lib7_Sig_maximum_valid_portable_signal_id,		"Void -> Int")



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

