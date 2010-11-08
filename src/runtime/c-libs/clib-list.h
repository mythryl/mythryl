/* clib-list.h
 *
 */

C_LIBRARY(Lib7_RunT_Library)
C_LIBRARY(Lib7_Sig_Library)
C_LIBRARY(Lib7_Prof_Library)

/* basis libraries */
C_LIBRARY(Lib7_Time_Library)
C_LIBRARY(Lib7_Date_Library)
C_LIBRARY(Lib7_Math_Library)
C_LIBRARY(Lib7_Sock_Library)

C_LIBRARY(Lib7_Gtk_Library)
C_LIBRARY(Lib7_Ncurses_Library)
C_LIBRARY(Lib7_OpenCV_Library)

#ifdef HAS_POSIX_LIBRARIES
C_LIBRARY(POSIX_Error_Library)
C_LIBRARY(POSIX_FileSys_Library)
C_LIBRARY(POSIX_IO_Library)
C_LIBRARY(POSIX_ProcEnv_Library)
C_LIBRARY(POSIX_Process_Library)
C_LIBRARY(POSIX_Signal_Library)
C_LIBRARY(POSIX_SysDB_Library)
C_LIBRARY(POSIX_TTY_Library)
#endif

#ifdef OPSYS_UNIX
C_LIBRARY(POSIX_OS_Library)
#elif defined(OPSYS_WIN32)
C_LIBRARY(WIN32_Library)
C_LIBRARY(WIN32_IO_Library)
C_LIBRARY(WIN32_FileSys_Library)
C_LIBRARY(WIN32_Process_Library)
#endif

#ifdef MP_SUPPORT
C_LIBRARY(Lib7_MP_Library)
#endif

#ifdef C_CALLS
C_LIBRARY(Lib7_CCalls_Library)
#endif

#ifdef BYTECODE
C_LIBRARY(Lib7_BC_Library)
#endif

#ifdef DLOPEN
C_LIBRARY(UNIX_Dynload_Library)
#endif


/* COPYRIGHT (c) 1994 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
