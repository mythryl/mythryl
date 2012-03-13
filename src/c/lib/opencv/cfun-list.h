// cfun-list.h
//
//
// C functions callable from Mythryl.
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"opencv"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"February 13, 2008"
#endif

CFUNC("create_image","create_image",			_lib7_OpenCV_cvCreateImage,	"String -> Image")
CFUNC("load_image","load_image",			_lib7_OpenCV_cvLoadImage,	"String -> Image")
CFUNC("make_random_number_generator","make_random_number_generator",	_lib7_OpenCV_cvRNG,		"Int -> Random_Number_Generator")
CFUNC("random_int","random_int",			_lib7_OpenCV_cvRandInt,		"Random_Number_Generator -> Int")
CFUNC("random_float","random_float",			_lib7_OpenCV_cvRandReal,	"Random_Number_Generator -> Float")





// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

