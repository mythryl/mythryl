/* win32-process.c
 *
 * interface to win32 process functions
 */

#include "../../config.h"

#include <windows.h>
#include <process.h>
#include <stdlib.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"


/* _lib7_win32_PS_create_process : String -> word32
 * 
 * Note: This function returns the handle to the created process
 *       This handle will need to be freed before the system releases
 *       the memory associated to the process.
 *       We will take care of this in the wait_for_single_chunk
 *       call. This is for the time being only used by threadkit.
 *       It could also cause problems later on.
 */
lib7_val_t _lib7_win32_PS_create_process(lib7_state_t *lib7_state, lib7_val_t arg)
{
  char *str = STR_LIB7toC(arg);
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  lib7_val_t res;
  BOOL fSuccess;
  ZeroMemory (&si,sizeof(si));
  si.cb = sizeof(si);
  fSuccess = CreateProcess (NULL,str,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi);
  if (fSuccess) {
    HANDLE hProcess = pi.hProcess;
    CloseHandle (pi.hThread);
    WORD_ALLOC (lib7_state,res,(Word_t)hProcess);
    return res;
  }
  WORD_ALLOC (lib7_state,res,(Word_t)0);
  return res;
}

lib7_val_t _lib7_win32_PS_wait_for_single_chunk(lib7_state_t *lib7_state, lib7_val_t arg)
{
  HANDLE hProcess = (HANDLE) WORD_LIB7toC (arg);
  DWORD exit_code;
  int res;
  lib7_val_t p,chunk;
  res = WaitForSingleChunkect (hProcess,0);
  if (res==WAIT_TIMEOUT || res==WAIT_FAILED) {
    /* information is not ready, or error */
    chunk = OPTION_NONE;
  }
  else { 
    /* WAIT_CHUNKECT_0 ... done, finished */
    /* get info and return THE(exit_status) */
    GetExitCodeProcess (hProcess,&exit_code);
    CloseHandle (hProcess);   /* decrease ref count */
    WORD_ALLOC (lib7_state,p,(Word_t)exit_code);
    OPTION_SOME(lib7_state,chunk,p);
  }
  return chunk;
}  
    

/* _lib7_win32_PS_system : String -> word32
 *                       command
 *
 */
lib7_val_t _lib7_win32_PS_system(lib7_state_t *lib7_state, lib7_val_t arg)
{
  int ret = system(STR_LIB7toC(arg));
  lib7_val_t res;

  WORD_ALLOC(lib7_state, res, (Word_t)ret);
  return res;
}

/* _lib7_win32_PS_exit_process : word32 -> 'a
 *                             exit code
 *
 */
void _lib7_win32_PS_exit_process(lib7_state_t *lib7_state, lib7_val_t arg)
{
  ExitProcess((UINT)WORD_LIB7toC(arg));
}

/* _lib7_win32_PS_get_environment_variable : String -> String option
 *                                         var
 *
 */
lib7_val_t _lib7_win32_PS_get_environment_variable(lib7_state_t *lib7_state, lib7_val_t arg)
{
#define GEV_BUF_SZ 4096
  char buf[GEV_BUF_SZ];
  int ret = GetEnvironmentVariable(STR_LIB7toC(arg),buf,GEV_BUF_SZ);
  lib7_val_t ml_s,res = OPTION_NONE;

  if (ret > GEV_BUF_SZ) {
    return RAISE_SYSERR(lib7_state,-1);
  }
  if (ret > 0) {
    ml_s = LIB7_CString(lib7_state,buf);
    OPTION_SOME(lib7_state,res,ml_s);
  }
  return res;
#undef GEV_BUF_SZ
}

/* _lib7_win32_PS_sleep : word32 -> Void
 *
 * Suspend execution for interval in MILLIseconds.
 */
lib7_val_t _lib7_win32_PS_sleep (lib7_state_t *lib7_state, lib7_val_t arg)
{
  Sleep ((DWORD) WORD_LIB7toC(arg));
  return LIB7_void;
}

/* end of win32-process.c */


/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

