/* win32-process.c
 *
 * interface to win32 process functions
 */

#include "../../mythryl-config.h"

#include <windows.h>
#include <process.h>
#include <stdlib.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"


/* _lib7_win32_PS_create_process : String -> one_word_unt
 * 
 * Note: This function returns the handle to the created process
 *       This handle will need to be freed before the system releases
 *       the memory associated to the process.
 *       We will take care of this in the wait_for_single_chunk
 *       call. This is for the time being only used by threadkit.
 *       It could also cause problems later on.
 */
Val _lib7_win32_PS_create_process(Task *task, Val arg)
{
  char *str = HEAP_STRING_AS_C_STRING(arg);
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  BOOL fSuccess;
  ZeroMemory (&si,sizeof(si));
  si.cb = sizeof(si);
  fSuccess = CreateProcess (NULL,str,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi);
  if (fSuccess) {
    HANDLE hProcess = pi.hProcess;
    CloseHandle (pi.hThread);
    return  make_one_word_unt(task,  (Vunt) hProcess  );
  }
  return  make_one_word_unt(task,  (Vunt) 0  );
}

Val _lib7_win32_PS_wait_for_single_chunk(Task *task, Val arg)
{
  HANDLE hProcess = (HANDLE) WORD_LIB7toC (arg);
  DWORD exit_code;
  int res;
  Val p;
  res = WaitForSingleChunkect (hProcess,0);
  if (res==WAIT_TIMEOUT || res==WAIT_FAILED) {
      /* information is not ready, or error */
      return OPTION_NULL;
  } else { 
      /* WAIT_CHUNKECT_0 ... done, finished */
      /* get info and return THE(exit_status) */
      GetExitCodeProcess (hProcess,&exit_code);
      CloseHandle (hProcess);						/* decrease ref count */
      p =  make_one_word_unt(task,  (Vunt) exit_code  );
      return OPTION_THE( task, p );
  }
}  
    

/* _lib7_win32_PS_system : String -> one_word_unt
 *                       command
 *
 */
Val _lib7_win32_PS_system(Task *task, Val arg)
{
  int ret = system(HEAP_STRING_AS_C_STRING(arg));
  //
  return make_one_word_unt(task,  (Vunt) ret  );
}

/* _lib7_win32_PS_exit_process : one_word_unt -> 'a
 *                             exit code
 *
 */
void _lib7_win32_PS_exit_process(Task *task, Val arg)
{
  ExitProcess((UINT)WORD_LIB7toC(arg));
}

/* _lib7_win32_PS_get_environment_variable : String -> String option
 *                                         var
 *
 */
Val _lib7_win32_PS_get_environment_variable(Task *task, Val arg)
{
#define GEV_BUF_SZ 4096
    char buf[GEV_BUF_SZ];
    int ret = GetEnvironmentVariable(HEAP_STRING_AS_C_STRING(arg),buf,GEV_BUF_SZ);
    Val ml_s;

    if (ret > GEV_BUF_SZ) {
	return RAISE_SYSERR__MAY_HEAPCLEAN(task,-1,NULL);
    }
    if (ret > 0) {
	ml_s = make_ascii_string_from_c_string__may_heapclean(task,buf,NULL);
	return OPTION_THE( task, ml_s );
    }
    return OPTION_NULL;
#undef GEV_BUF_SZ
}

/* _lib7_win32_PS_sleep : one_word_unt -> Void
 *
 * Suspend execution for interval in MILLIseconds.
 */
Val _lib7_win32_PS_sleep (Task *task, Val arg)
{
  Sleep ((DWORD) WORD_LIB7toC(arg));
  return HEAP_VOID;
}

/* end of win32-process.c */


/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
 * released under Gnu Public Licence version 3.
 */

