// cfun-list.h
//
// This file lists the "multicore" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my acquire_pthread:   (Thread, Fate) -> Bool
//         =
//         mythryl_callable_c_library_interface::find_c_function { lib_name => "multicore", fun_name => "acquire_pthread" };
// 
// or such.
// It gets #included by both:
//
//     src/c/lib/multicore/libmythryl-multicore.c
//     src/c/lib/multicore/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c

#ifndef CLIB_NAME
#define CLIB_NAME	"multicore"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"December 18, 1994"
#endif

CFUNC("acquire_pthread","acquire_pthread",	_lib7_MP_acquire_pthread,	"")		// Defined in   src/c/lib/multicore/acquire-pthread.c 
CFUNC("max_pthreads","max_pthreads",		_lib7_MP_max_pthreads,		"")		// Defined in   src/c/lib/multicore/max-pthreads.c 
CFUNC("release_pthread","release_pthread",	_lib7_MP_release_pthread,	"")		// Defined in   src/c/lib/multicore/release-pthread.c 
CFUNC("spin_lock","spin_lock",			_lib7_MP_spin_lock,		"")		// Defined in   src/c/lib/multicore/spin-lock.c 



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

