/* win32-fault.h
 *
 *
 */

extern HANDLE win32_stdin_handle;
extern HANDLE win32_stdout_handle;
extern HANDLE win32_stderr_handle;

extern HANDLE win32_LIB7_microthread;

extern void wait_for_cntrl_c(void);
extern BOOL win32_generic_handler(int code);

extern BOOL win32_isNT;

/* end of win32-fault.h */



/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
 * released per terms of SMLNJ-COPYRIGHT.
 */
