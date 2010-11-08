/* win32-filesys.c
 *
 * interface to win32 filesys functions
 */

#include "../../config.h"

#include <windows.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"

#define TMP_PREFIX "TMP-LIB7"

#define IS_DOTDIR(c) ((c)[0] == '.' && (!(c)[1] || ((c)[1] == '.' && !(c)[2])))

static WIN32_FIND_DATA wfd;

static lib7_val_t find_next_file(lib7_state_t *lib7_state,HANDLE h)
{
  lib7_val_t fname_opt,fname;

loop:
  if (FindNextFile(h,&wfd)) {
    if (IS_DOTDIR(wfd.cFileName))
      goto loop;
    fname = LIB7_CString(lib7_state,wfd.cFileName);
    OPTION_SOME(lib7_state,fname_opt,fname);
  } else
    fname_opt = OPTION_NONE;
  return fname_opt;
}
    
/* _lib7_win32_FS_find_next_file: word32 -> (String option)
 */
lib7_val_t _lib7_win32_FS_find_next_file(lib7_state_t *lib7_state, lib7_val_t arg)
{
  HANDLE h = (HANDLE) WORD_LIB7toC(arg);

  return find_next_file(lib7_state,h);
}

/* _lib7_win32_FS_find_first_file: String -> (word32 * String option)
 */
lib7_val_t _lib7_win32_FS_find_first_file(lib7_state_t *lib7_state, lib7_val_t arg)
{
  HANDLE h = FindFirstFile(STR_LIB7toC(arg),&wfd);
  lib7_val_t fname_opt, fname, w, res;

  if (h != INVALID_HANDLE_VALUE) {
    if (IS_DOTDIR(wfd.cFileName))
      fname_opt = find_next_file(lib7_state,h);
    else {
      fname = LIB7_CString(lib7_state,wfd.cFileName);
      OPTION_SOME(lib7_state,fname_opt,fname);
    }
  } else
    fname_opt = OPTION_NONE;
  WORD_ALLOC(lib7_state, w, (Word_t)h);
  REC_ALLOC2(lib7_state,res,w,fname_opt);
  return res;
}

/* _lib7_win32_FS_find_close: word32 -> Bool
 */
lib7_val_t _lib7_win32_FS_find_close(lib7_state_t *lib7_state, lib7_val_t arg)
{
  return FindClose((HANDLE)WORD_LIB7toC(arg)) ? LIB7_true : LIB7_false;
}

/* _lib7_win32_FS_set_current_directory: String -> Bool
 */
lib7_val_t _lib7_win32_FS_set_current_directory(lib7_state_t *lib7_state, lib7_val_t arg)
{
  return SetCurrentDirectory(STR_LIB7toC(arg)) ? LIB7_true : LIB7_false;
}

/* _lib7_win32_FS_get_current_directory: Void -> String
 */
lib7_val_t _lib7_win32_FS_get_current_directory(lib7_state_t *lib7_state, lib7_val_t arg)
{
  char buf[MAX_PATH];
  DWORD r = GetCurrentDirectory(MAX_PATH,buf);

  if (r == 0 || r > MAX_PATH) {
      return RAISE_SYSERR(lib7_state,-1);
  }
  return LIB7_CString(lib7_state,buf);
}


/* _lib7_win32_FS_create_directory: String -> Bool
 */
lib7_val_t _lib7_win32_FS_create_directory(lib7_state_t *lib7_state, lib7_val_t arg)
{
  return CreateDirectory(STR_LIB7toC(arg),NULL) ? LIB7_true : LIB7_false;
}

/* _lib7_win32_FS_remove_directory: String -> Bool
 */
lib7_val_t _lib7_win32_FS_remove_directory(lib7_state_t *lib7_state, lib7_val_t arg)
{
  return RemoveDirectory(STR_LIB7toC(arg)) ? LIB7_true : LIB7_false;
}

/* _lib7_win32_FS_get_file_attributes: String -> (word32 option)
 */
lib7_val_t _lib7_win32_FS_get_file_attributes(lib7_state_t *lib7_state, lib7_val_t arg)
{
  DWORD w = GetFileAttributes(STR_LIB7toC(arg));
  lib7_val_t res, ml_w;

  if (w != 0xffffffff) {
#ifdef DEBUG_WIN32
    printf("_lib7_win32_FS_get_file_attributes: returning file attributes for <%s> as THE %x\n",STR_LIB7toC(arg),w);
#endif
    WORD_ALLOC(lib7_state,ml_w,w);
    OPTION_SOME(lib7_state,res,ml_w);
  } else {
#ifdef DEBUG_WIN32
    printf("returning NULL as attributes for <%s>\n",STR_LIB7toC(arg));
#endif
    res = OPTION_NONE;
  }
  return res;
}

/* _lib7_win32_FS_get_file_attributes_by_handle: word32 -> (word32 option)
 */
lib7_val_t _lib7_win32_FS_get_file_attributes_by_handle(lib7_state_t *lib7_state, lib7_val_t arg)
{
  BY_HANDLE_FILE_INFORMATION bhfi;
  lib7_val_t ml_w, res;

  if (GetFileInformationByHandle((HANDLE)WORD_LIB7toC(arg),&bhfi)) {
    WORD_ALLOC(lib7_state,ml_w,bhfi.dwFileAttributes);
    OPTION_SOME(lib7_state,res,ml_w);
  } else {
    res = OPTION_NONE;
  }
  return res;
}

/* _lib7_win32_FS_get_full_path_name: String -> String
 */
lib7_val_t _lib7_win32_FS_get_full_path_name(lib7_state_t *lib7_state, lib7_val_t arg)
{
  char buf[MAX_PATH], *dummy;
  DWORD r;
  lib7_val_t res;

  r = GetFullPathName(STR_LIB7toC(arg),MAX_PATH,buf,&dummy);
  if (r == 0 | r > MAX_PATH) {
      return  RAISE_SYSERR(lib7_state,-1);
  }
  res = LIB7_CString(lib7_state,buf);
  return res;
}

/* _lib7_win32_FS_get_file_size: word32 -> (word32 * word32)
 */
lib7_val_t _lib7_win32_FS_get_file_size(lib7_state_t *lib7_state, lib7_val_t arg)
{
  DWORD lo,hi;
  lib7_val_t ml_lo, ml_hi, res;

  lo = GetFileSize((HANDLE)WORD_LIB7toC(arg),&hi);
  WORD_ALLOC(lib7_state,ml_lo,lo);
  WORD_ALLOC(lib7_state,ml_hi,hi);
  REC_ALLOC2(lib7_state,res,ml_hi,ml_lo);
  return res;
}

/* _lib7_win32_FS_get_low_file_size: word32 -> (word32 option)
 */
lib7_val_t _lib7_win32_FS_get_low_file_size(lib7_state_t *lib7_state, lib7_val_t arg)
{
  DWORD lo;
  lib7_val_t ml_lo, res;

  lo = GetFileSize((HANDLE)WORD_LIB7toC(arg),NULL);
  if (lo != 0xffffffff) {
    WORD_ALLOC(lib7_state,ml_lo,lo);
    OPTION_SOME(lib7_state,res,ml_lo);
  } else {
    res = OPTION_NONE;
  }
  return res;
}

/* _lib7_win32_FS_get_low_file_size_by_name: String -> (word32 option)
 */
lib7_val_t _lib7_win32_FS_get_low_file_size_by_name(lib7_state_t *lib7_state, lib7_val_t arg)
{
  HANDLE h;
  lib7_val_t res = OPTION_NONE;

  h = CreateFile(STR_LIB7toC(arg),0,0,NULL,
		 OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,INVALID_HANDLE_VALUE);
  if (h != INVALID_HANDLE_VALUE) {
    DWORD lo;
    lib7_val_t ml_lo;

    lo = GetFileSize(h,NULL);
    CloseHandle(h);
    if (lo != 0xffffffff) {
      WORD_ALLOC(lib7_state,ml_lo,lo);
      OPTION_SOME(lib7_state,res,ml_lo);
    }
  }
  return res;
}

#define REC_ALLOC8(lib7_state, r, a, b, c, d, e, f, g, h)	{	\
	lib7_state_t	*__lib7_state = (lib7_state);				\
	lib7_val_t	*__p = __lib7_state->lib7_heap_cursor;		\
	*__p++ = MAKE_DESC(8, DTAG_record);			\
	*__p++ = (a);						\
	*__p++ = (b);						\
	*__p++ = (c);						\
	*__p++ = (d);						\
	*__p++ = (e);						\
	*__p++ = (f);						\
	*__p++ = (g);						\
	*__p++ = (h);						\
	(r) = PTR_CtoLib7(__lib7_state->lib7_heap_cursor + 1);		\
	__lib7_state->lib7_heap_cursor = __p;				\
    }

/* _lib7_win32_FS_get_file_time: 
 *   String -> (int * int * int * int * int * int * int * int) option
 *              year  month wday  day   hour  min   sec   ms
 */
lib7_val_t _lib7_win32_FS_get_file_time(lib7_state_t *lib7_state, lib7_val_t arg)
{
  HANDLE h;
  lib7_val_t res = OPTION_NONE;

  h = CreateFile(STR_LIB7toC(arg),0,0,NULL,
		 OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,INVALID_HANDLE_VALUE);
  if (h != INVALID_HANDLE_VALUE) {
    FILETIME ft;

    if (GetFileTime(h,NULL,NULL,&ft)) {  /* request time of "last write" */
      SYSTEMTIME st;
    
      CloseHandle(h);
      if (FileTimeToSystemTime(&ft,&st)) {
	lib7_val_t rec;

	REC_ALLOC8(lib7_state,rec,
		   INT_CtoLib7((int)st.wYear),
		   INT_CtoLib7((int)st.wMonth),
		   INT_CtoLib7((int)st.wDayOfWeek),
		   INT_CtoLib7((int)st.wDay),
		   INT_CtoLib7((int)st.wHour),
		   INT_CtoLib7((int)st.wMinute),
		   INT_CtoLib7((int)st.wSecond),
		   INT_CtoLib7((int)st.wMilliseconds));
	OPTION_SOME(lib7_state,res,rec);
      }
    }
  }
  return res;
}

/* _lib7_win32_FS_set_file_time: 
 *   (String * (int * int * int * int * int * int * int * int) option) -> Bool
 *              year  month wday  day   hour  min   sec   ms
 */
lib7_val_t _lib7_win32_FS_set_file_time(lib7_state_t *lib7_state, lib7_val_t arg)
{
  HANDLE	h;
  lib7_val_t	res = LIB7_false;
  lib7_val_t	fname = REC_SEL(arg,0);
  lib7_val_t	time_rec = REC_SEL(arg,1);

  h = CreateFile (
	STR_LIB7toC(fname), GENERIC_WRITE, 0 ,NULL,
	OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);

  if (h != INVALID_HANDLE_VALUE) {
    FILETIME ft;
    SYSTEMTIME st;

    st.wYear = REC_SELINT(time_rec,0);
    st.wMonth = REC_SELINT(time_rec,1);
    st.wDayOfWeek = REC_SELINT(time_rec,2);
    st.wDay = REC_SELINT(time_rec,3);
    st.wHour = REC_SELINT(time_rec,4);
    st.wMinute = REC_SELINT(time_rec,5);
    st.wSecond = REC_SELINT(time_rec,6);
    st.wMilliseconds = REC_SELINT(time_rec,7);
    
    if (SystemTimeToFileTime(&st,&ft) && SetFileTime(h,NULL,NULL,&ft)) {
      res = LIB7_true;
    }
    
    CloseHandle(h);
  }
  return res;
}

/* _lib7_win32_FS_delete_file: String->Bool
 */
lib7_val_t _lib7_win32_FS_delete_file(lib7_state_t *lib7_state, lib7_val_t arg)
{
  return DeleteFile(STR_LIB7toC(arg)) ? LIB7_true : LIB7_false;
}

/* _lib7_win32_FS_move_file: (String * String)->Bool
 */
lib7_val_t _lib7_win32_FS_move_file(lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	f1 = REC_SEL(arg, 0);
    lib7_val_t	f2 = REC_SEL(arg, 1);

    if (MoveFile (STR_LIB7toC(f1), STR_LIB7toC(f2)))
	return LIB7_true;
    else
	return LIB7_false;
}

/* _lib7_win32_FS_get_temp_file_name: Void -> String option
 */
lib7_val_t _lib7_win32_FS_get_temp_file_name(lib7_state_t *lib7_state, lib7_val_t arg)
{
  lib7_val_t res = OPTION_NONE;
  char name_buf[MAX_PATH];
  char path_buf[MAX_PATH];
  DWORD pblen;

  pblen = GetTempPath(MAX_PATH,path_buf);
  if ((pblen <= MAX_PATH) && 
      (GetTempFileName(path_buf,TMP_PREFIX,0,name_buf) != 0)) {
    lib7_val_t tfn = LIB7_CString(lib7_state,name_buf);
    
    OPTION_SOME(lib7_state,res,tfn);
  }
  return res;
}

/* end of win32-filesys.c */


/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

