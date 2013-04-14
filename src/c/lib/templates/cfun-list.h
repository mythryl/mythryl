// cfun-list.h
//
//
// C functions callable from Mythryl.
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"<<your library name here>>"
#define CLIB_VERSION	"<<version tag>>"
#define CLIB_DATE	"<<version date>>"
#endif

CFUNC("<<Mythryl function name>>",	<<C function name>>,		"<<Mythryl type (optional)>>")



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

