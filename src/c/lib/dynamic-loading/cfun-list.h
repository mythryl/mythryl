// cfun-list.h
//
// C functions callable from Mythryl.
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"dynamic_loading"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"January 1, 2001"
#endif

CFUNC("dlopen","dlopen",  _lib7_U_Dynload_dlopen,  "(Null_Or(String), Bool, Bool) -> unt1::Unt")
CFUNC("dlsym","dlsym",   _lib7_U_Dynload_dlsym,   "(unt1::Unt, String) -> unt1::Unt")
CFUNC("dlclose","dlclose", _lib7_U_Dynload_dlclose, "unt1::Unt -> Void")
CFUNC("dlerror","dlerror", _lib7_U_Dynload_dlerror, "Void -> Null_Or(String)")


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


