/* cfun-list.h
 *
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"Lib7-OpenCV"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"February 13, 2008"
#endif

CFUNC("create_image",			_lib7_OpenCV_cvCreateImage,	"String -> Image")
CFUNC("load_image",			_lib7_OpenCV_cvLoadImage,	"String -> Image")
CFUNC("make_random_number_generator",	_lib7_OpenCV_cvRNG,		"Int -> Random_Number_Generator")
CFUNC("random_int",			_lib7_OpenCV_cvRandInt,		"Random_Number_Generator -> Int")
CFUNC("random_float",			_lib7_OpenCV_cvRandReal,	"Random_Number_Generator -> Float")





/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
