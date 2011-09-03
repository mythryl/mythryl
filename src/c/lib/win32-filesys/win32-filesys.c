// win32-filesys.c
//
// interface to win32 filesys functions


#include "../../config.h"

#include <windows.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"

#define TMP_PREFIX "TMP-LIB7"

#define IS_DOTDIR(c) ((c)[0] == '.' && (!(c)[1] || ((c)[1] == '.' && !(c)[2])))

static WIN32_FIND_DATA wfd;

static Val   find_next_file   (Task* task, HANDLE h)   {
    //       ==============
    //
    Val fname_opt;
    Val fname;

loop:
    if (FindNextFile(h,&wfd)) {
	//
	if (IS_DOTDIR(wfd.cFileName))   goto loop;
	//
	fname = make_ascii_string_from_c_string(task,wfd.cFileName);
	//
	OPTION_THE(task,fname_opt,fname);
	//
    } else {
	//
        fname_opt = OPTION_NULL;
    }
    return fname_opt;
}
    


Val   _lib7_win32_FS_find_next_file   (Task* task,  Val arg)   {
    //=============================
    //
    // Mythryl type:   Unt1 -> Null_Or(String)
    //
    HANDLE h = (HANDLE) WORD_LIB7toC(arg);
    //
    return find_next_file(task,h);
}



Val   _lib7_win32_FS_find_first_file   (Task* task,  Val arg)   {
    //==============================
    //
    // Mythryl type:   String ->  (Unt1, Null_Or(String))

    HANDLE h = FindFirstFile(HEAP_STRING_AS_C_STRING(arg),&wfd);
    Val fname_opt, fname, w, res;

    if (h != INVALID_HANDLE_VALUE) {
      if (IS_DOTDIR(wfd.cFileName))
	fname_opt = find_next_file(task,h);
      else {
	fname = make_ascii_string_from_c_string(task,wfd.cFileName);
	OPTION_THE(task,fname_opt,fname);
      }
    } else
      fname_opt = OPTION_NULL;
    WORD_ALLOC(task, w, (Val_Sized_Unt)h);
    REC_ALLOC2(task,res,w,fname_opt);
    return res;
}

/* _lib7_win32_FS_find_close: unt1 -> Bool
 */
Val _lib7_win32_FS_find_close(Task *task, Val arg)
{
  return FindClose((HANDLE)WORD_LIB7toC(arg)) ? HEAP_TRUE : HEAP_FALSE;
}

/* _lib7_win32_FS_set_current_directory: String -> Bool
 */
Val _lib7_win32_FS_set_current_directory(Task *task, Val arg)
{
  return SetCurrentDirectory(HEAP_STRING_AS_C_STRING(arg)) ? HEAP_TRUE : HEAP_FALSE;
}

/* _lib7_win32_FS_get_current_directory: Void -> String
 */
Val _lib7_win32_FS_get_current_directory(Task *task, Val arg)
{
  char buf[MAX_PATH];
  DWORD r = GetCurrentDirectory(MAX_PATH,buf);

  if (r == 0 || r > MAX_PATH) {
      return RAISE_SYSERR(task,-1);
  }
  return make_ascii_string_from_c_string(task,buf);
}


/* _lib7_win32_FS_create_directory: String -> Bool
 */
Val _lib7_win32_FS_create_directory(Task *task, Val arg)
{
  return CreateDirectory(HEAP_STRING_AS_C_STRING(arg),NULL) ? HEAP_TRUE : HEAP_FALSE;
}

/* _lib7_win32_FS_remove_directory: String -> Bool
 */
Val _lib7_win32_FS_remove_directory(Task *task, Val arg)
{
  return RemoveDirectory(HEAP_STRING_AS_C_STRING(arg)) ? HEAP_TRUE : HEAP_FALSE;
}

/* _lib7_win32_FS_get_file_attributes: String -> (unt1 option)
 */
Val _lib7_win32_FS_get_file_attributes(Task *task, Val arg)
{
  DWORD w = GetFileAttributes(HEAP_STRING_AS_C_STRING(arg));
  Val res, ml_w;

  if (w != 0xffffffff) {
#ifdef DEBUG_WIN32
    printf("_lib7_win32_FS_get_file_attributes: returning file attributes for <%s> as THE %x\n",HEAP_STRING_AS_C_STRING(arg),w);
#endif
    WORD_ALLOC(task,ml_w,w);
    OPTION_THE(task,res,ml_w);
  } else {
#ifdef DEBUG_WIN32
    printf("returning NULL as attributes for <%s>\n",HEAP_STRING_AS_C_STRING(arg));
#endif
    res = OPTION_NULL;
  }
  return res;
}

/* _lib7_win32_FS_get_file_attributes_by_handle: unt1 -> (unt1 option)
 */
Val _lib7_win32_FS_get_file_attributes_by_handle(Task *task, Val arg)
{
  BY_HANDLE_FILE_INFORMATION bhfi;
  Val ml_w, res;

  if (GetFileInformationByHandle((HANDLE)WORD_LIB7toC(arg),&bhfi)) {
    WORD_ALLOC(task,ml_w,bhfi.dwFileAttributes);
    OPTION_THE(task,res,ml_w);
  } else {
    res = OPTION_NULL;
  }
  return res;
}

/* _lib7_win32_FS_get_full_path_name: String -> String
 */
Val _lib7_win32_FS_get_full_path_name(Task *task, Val arg)
{
  char buf[MAX_PATH], *dummy;
  DWORD r;
  Val res;

  r = GetFullPathName(HEAP_STRING_AS_C_STRING(arg),MAX_PATH,buf,&dummy);
  if (r == 0 | r > MAX_PATH) {
      return  RAISE_SYSERR(task,-1);
  }
  res = make_ascii_string_from_c_string(task,buf);
  return res;
}

/* _lib7_win32_FS_get_file_size: unt1 -> (unt1 * unt1)
 */
Val _lib7_win32_FS_get_file_size(Task *task, Val arg)
{
  DWORD lo,hi;
  Val ml_lo, ml_hi, res;

  lo = GetFileSize((HANDLE)WORD_LIB7toC(arg),&hi);
  WORD_ALLOC(task,ml_lo,lo);
  WORD_ALLOC(task,ml_hi,hi);
  REC_ALLOC2(task,res,ml_hi,ml_lo);
  return res;
}

/* _lib7_win32_FS_get_low_file_size: unt1 -> (unt1 option)
 */
Val _lib7_win32_FS_get_low_file_size(Task *task, Val arg)
{
  DWORD lo;
  Val ml_lo, res;

  lo = GetFileSize((HANDLE)WORD_LIB7toC(arg),NULL);
  if (lo != 0xffffffff) {
    WORD_ALLOC(task,ml_lo,lo);
    OPTION_THE(task,res,ml_lo);
  } else {
    res = OPTION_NULL;
  }
  return res;
}

/* _lib7_win32_FS_get_low_file_size_by_name: String -> (unt1 option)
 */
Val _lib7_win32_FS_get_low_file_size_by_name(Task *task, Val arg)
{
  HANDLE h;
  Val res = OPTION_NULL;

  h = CreateFile(HEAP_STRING_AS_C_STRING(arg),0,0,NULL,
		 OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,INVALID_HANDLE_VALUE);
  if (h != INVALID_HANDLE_VALUE) {
    DWORD lo;
    Val ml_lo;

    lo = GetFileSize(h,NULL);
    CloseHandle(h);
    if (lo != 0xffffffff) {
      WORD_ALLOC(task,ml_lo,lo);
      OPTION_THE(task,res,ml_lo);
    }
  }
  return res;
}

#define REC_ALLOC8(task, r, a, b, c, d, e, f, g, h)	{	\
	Task	*__task = (task);				\
	Val	*__p = __task->heap_allocation_pointer;		\
	*__p++ = MAKE_TAGWORD(8, PAIRS_AND_RECORDS_BTAG);			\
	*__p++ = (a);						\
	*__p++ = (b);						\
	*__p++ = (c);						\
	*__p++ = (d);						\
	*__p++ = (e);						\
	*__p++ = (f);						\
	*__p++ = (g);						\
	*__p++ = (h);						\
	(r) = PTR_CAST( Val, __task->heap_allocation_pointer + 1);		\
	__task->heap_allocation_pointer = __p;				\
    }

/* _lib7_win32_FS_get_file_time: 
 *   String -> (int * int * int * int * int * int * int * int) option
 *              year  month wday  day   hour  min   sec   ms
 */
Val _lib7_win32_FS_get_file_time(Task *task, Val arg)
{
  HANDLE h;
  Val res = OPTION_NULL;

  h = CreateFile(HEAP_STRING_AS_C_STRING(arg),0,0,NULL,
		 OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,INVALID_HANDLE_VALUE);
  if (h != INVALID_HANDLE_VALUE) {
    FILETIME ft;

    if (GetFileTime(h,NULL,NULL,&ft)) {  /* request time of "last write" */
      SYSTEMTIME st;
    
      CloseHandle(h);
      if (FileTimeToSystemTime(&ft,&st)) {
	Val rec;

	REC_ALLOC8(task,rec,
		   TAGGED_INT_FROM_C_INT((int)st.wYear),
		   TAGGED_INT_FROM_C_INT((int)st.wMonth),
		   TAGGED_INT_FROM_C_INT((int)st.wDayOfWeek),
		   TAGGED_INT_FROM_C_INT((int)st.wDay),
		   TAGGED_INT_FROM_C_INT((int)st.wHour),
		   TAGGED_INT_FROM_C_INT((int)st.wMinute),
		   TAGGED_INT_FROM_C_INT((int)st.wSecond),
		   TAGGED_INT_FROM_C_INT((int)st.wMilliseconds));
	OPTION_THE(task,res,rec);
      }
    }
  }
  return res;
}

/* _lib7_win32_FS_set_file_time: 
 *   (String * (int * int * int * int * int * int * int * int) option) -> Bool
 *              year  month wday  day   hour  min   sec   ms
 */
Val _lib7_win32_FS_set_file_time(Task *task, Val arg)
{
  HANDLE	h;
  Val	res = HEAP_FALSE;
  Val	fname = GET_TUPLE_SLOT_AS_VAL(arg,0);
  Val	time_rec = GET_TUPLE_SLOT_AS_VAL(arg,1);

  h = CreateFile (
	HEAP_STRING_AS_C_STRING(fname), GENERIC_WRITE, 0 ,NULL,
	OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);

  if (h != INVALID_HANDLE_VALUE) {
    FILETIME ft;
    SYSTEMTIME st;

    st.wYear = GET_TUPLE_SLOT_AS_INT(time_rec,0);
    st.wMonth = GET_TUPLE_SLOT_AS_INT(time_rec,1);
    st.wDayOfWeek = GET_TUPLE_SLOT_AS_INT(time_rec,2);
    st.wDay = GET_TUPLE_SLOT_AS_INT(time_rec,3);
    st.wHour = GET_TUPLE_SLOT_AS_INT(time_rec,4);
    st.wMinute = GET_TUPLE_SLOT_AS_INT(time_rec,5);
    st.wSecond = GET_TUPLE_SLOT_AS_INT(time_rec,6);
    st.wMilliseconds = GET_TUPLE_SLOT_AS_INT(time_rec,7);
    
    if (SystemTimeToFileTime(&st,&ft) && SetFileTime(h,NULL,NULL,&ft)) {
      res = HEAP_TRUE;
    }
    
    CloseHandle(h);
  }
  return res;
}

/* _lib7_win32_FS_delete_file: String->Bool
 */
Val _lib7_win32_FS_delete_file(Task *task, Val arg)
{
  return DeleteFile(HEAP_STRING_AS_C_STRING(arg)) ? HEAP_TRUE : HEAP_FALSE;
}

/* _lib7_win32_FS_move_file: (String * String)->Bool
 */
Val _lib7_win32_FS_move_file(Task *task, Val arg)
{
    Val	f1 = GET_TUPLE_SLOT_AS_VAL(arg, 0);
    Val	f2 = GET_TUPLE_SLOT_AS_VAL(arg, 1);

    if (MoveFile (HEAP_STRING_AS_C_STRING(f1), HEAP_STRING_AS_C_STRING(f2)))
	return HEAP_TRUE;
    else
	return HEAP_FALSE;
}

/* _lib7_win32_FS_get_temp_file_name: Void -> String option
 */
Val _lib7_win32_FS_get_temp_file_name(Task *task, Val arg)
{
  Val res = OPTION_NULL;
  char name_buf[MAX_PATH];
  char path_buf[MAX_PATH];
  DWORD pblen;

  pblen = GetTempPath(MAX_PATH,path_buf);
  if ((pblen <= MAX_PATH) && 
      (GetTempFileName(path_buf,TMP_PREFIX,0,name_buf) != 0)) {
    Val tfn = make_ascii_string_from_c_string(task,name_buf);
    
    OPTION_THE(task,res,tfn);
  }
  return res;
}

/* end of win32-filesys.c */


/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
 * released under Gnu Public Licence version 3.
 */

