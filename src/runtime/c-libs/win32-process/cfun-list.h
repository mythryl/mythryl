/* cfun-list.h
 *
 *
 * win32 C functions for processes
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"WIN32-PROCESS"
#define CLIB_VERSION	"0.2"
#define CLIB_DATE	"May 22, 1998"
#endif

CFUNC("system",_lib7_win32_PS_system,"String->word32")
CFUNC("exit_process",_lib7_win32_PS_exit_process,"word32->'a")
CFUNC("get_environment_variable",_lib7_win32_PS_get_environment_variable,"String->String option")
CFUNC("create_process",_lib7_win32_PS_create_process,"String->word32")
CFUNC("wait_for_single_chunk",_lib7_win32_PS_wait_for_single_chunk,"word32->word32 option")

CFUNC("sleep",_lib7_win32_PS_sleep,"word32->Void")


/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
