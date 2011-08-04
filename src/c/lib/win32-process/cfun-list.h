// cfun-list.h
//
//
// win32 C functions for processes
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"win32_process"
#define CLIB_VERSION	"0.2"
#define CLIB_DATE	"May 22, 1998"
#endif

CFUNC("system","system",_lib7_win32_PS_system,"String->unt32")
CFUNC("exit_process","exit_process",_lib7_win32_PS_exit_process,"unt32->'a")
CFUNC("get_environment_variable","get_environment_variable",_lib7_win32_PS_get_environment_variable,"String->String option")
CFUNC("create_process","create_process",_lib7_win32_PS_create_process,"String->unt32")
CFUNC("wait_for_single_chunk","wait_for_single_chunk",_lib7_win32_PS_wait_for_single_chunk,"unt32->unt32 option")

CFUNC("sleep","sleep",_lib7_win32_PS_sleep,"unt32->Void")


// COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

