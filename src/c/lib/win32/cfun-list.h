// cfun-list.h
//
//
// Utility win32 C functions
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"win32"
#define CLIB_VERSION	"0.1"
#define CLIB_DATE	"October 11, 1996"
#endif

CFUNC("get_const","get_const",	   _lib7_win32_get_const,	"String -> one_word_unt")
CFUNC("get_last_error","get_last_error",    _lib7_win32_get_last_error, "Void -> one_word_unt")
CFUNC("debug","debug",             _lib7_win32_debug, "String -> Void")


// COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

