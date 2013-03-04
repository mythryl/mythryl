// win32-file-system.c
//
// interface to win32 file-system functions


#include "../../mythryl-config.h"

#include <windows.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"

#define TMP_PREFIX "TMP-LIB7"

#define IS_DOTDIR(c) ((c)[0] == '.' && (!(c)[1] || ((c)[1] == '.' && !(c)[2])))

static WIN32_FIND_DATA wfd;

static Val   find_next_file__may_heapclean   (Task* task, HANDLE h, Roots* extra_roots)   {
    //       =============================
    //
    Val fname;

loop:
    if (FindNextFile(h,&wfd)) {
	//
	if (IS_DOTDIR(wfd.cFileName))   goto loop;
	//
	fname = make_ascii_string_from_c_string__may_heapclean(task, wfd.cFileName, extra_roots);
	//
	return OPTION_THE( task, fname );
	//
    } else {
	//
        return OPTION_NULL;
    }
}
    


Val   _lib7_win32_FS_find_next_file   (Task* task,  Val arg)   {
    //=============================
    //
    // Mythryl type:   Unt1 -> Null_Or(String)
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    HANDLE h = (HANDLE) WORD_LIB7toC(arg);
    //
    Val result = find_next_file__may_heapclean(task,h,NULL);
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}



Val   _lib7_win32_FS_find_first_file   (Task* task,  Val arg)   {
    //==============================
    //
    // Mythryl type:   String ->  (Unt1, Null_Or(String))

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    HANDLE h = FindFirstFile(HEAP_STRING_AS_C_STRING(arg),&wfd);
    Val fname_opt, fname, w;
    Val result;
    if (h != INVALID_HANDLE_VALUE) {
      if (IS_DOTDIR(wfd.cFileName))
	fname_opt = find_next_file__may_heapclean(task,h,NULL);
      else {
	fname = make_ascii_string_from_c_string__may_heapclean(task,wfd.cFileName, NULL );
	fname_opt = OPTION_THE( task, fname );
      }
    } else {
      fname_opt = OPTION_NULL;
    }

    w =  make_one_word_unt(task,  (Vunt) h  );

    Val result =  make_two_slot_record(task,  w, fname_opt  );
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

/* _lib7_win32_FS_find_close: one_word_unt -> Bool
 */
Val _lib7_win32_FS_find_close(Task *task, Val arg)
{
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
  Val result = FindClose((HANDLE)WORD_LIB7toC(arg)) ? HEAP_TRUE : HEAP_FALSE;
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
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
      return RAISE_SYSERR__MAY_HEAPCLEAN(task,-1,NULL);
  }
  return make_ascii_string_from_c_string__may_heapclean(task,buf,NULL);
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

/* _lib7_win32_FS_get_file_attributes: String -> (one_word_unt option)
 */
Val _lib7_win32_FS_get_file_attributes(Task *task, Val arg)
{
  DWORD w = GetFileAttributes(HEAP_STRING_AS_C_STRING(arg));
  Val ml_w;

  if (w != 0xffffffff) {
#ifdef DEBUG_WIN32
    printf("_lib7_win32_FS_get_file_attributes: returning file attributes for <%s> as THE %x\n",HEAP_STRING_AS_C_STRING(arg),w); fflush(stdout);
#endif
    ml_w =  make_one_word_unt(task, w );
    return OPTION_THE(task,ml_w);
  } else {
#ifdef DEBUG_WIN32
    printf("returning NULL as attributes for <%s>\n",HEAP_STRING_AS_C_STRING(arg)); fflush(stdout);
#endif
    return OPTION_NULL;
  }
}

/* _lib7_win32_FS_get_file_attributes_by_handle: one_word_unt -> (one_word_unt option)
 */
Val _lib7_win32_FS_get_file_attributes_by_handle(Task *task, Val arg)
{
    BY_HANDLE_FILE_INFORMATION bhfi;
    Val ml_w;

    if (GetFileInformationByHandle((HANDLE)WORD_LIB7toC(arg),&bhfi)) {
	//
	ml_w =  make_one_word_unt(task,  bhfi.dwFileAttributes  );
	return OPTION_THE( task, ml_w );
    } else {
	return  OPTION_NULL;
    }
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
      return  RAISE_SYSERR__MAY_HEAPCLEAN(task,-1,NULL);
  }
  res = make_ascii_string_from_c_string__may_heapclean(task,buf,NULL);
  return res;
}

/* _lib7_win32_FS_get_file_size: one_word_unt -> (one_word_unt * one_word_unt)
 */
Val _lib7_win32_FS_get_file_size(Task *task, Val arg)
{
    DWORD lo,hi;
    Val ml_lo, ml_hi, result;

    lo = GetFileSize((HANDLE)WORD_LIB7toC(arg),&hi);

    ml_lo =  make_one_word_unt(task,  lo  );
    ml_hi =  make_one_word_unt(task,  hi  );

    return  make_two_slot_record(task,  ml_hi, ml_lo  );
}

/* _lib7_win32_FS_get_low_file_size: one_word_unt -> (one_word_unt option)
 */
Val _lib7_win32_FS_get_low_file_size(Task *task, Val arg)
{
    DWORD lo;
    Val ml_lo;

    lo = GetFileSize((HANDLE)WORD_LIB7toC(arg),NULL);
    if (lo != 0xffffffff) {
	ml_lo =  make_one_word_unt(task,  lo  );
	return OPTION_THE( task, ml_lo );
    } else {
	return OPTION_NULL;
    }
}

// _lib7_win32_FS_get_low_file_size_by_name: String -> (one_word_unt option)
//
Val _lib7_win32_FS_get_low_file_size_by_name(Task *task, Val arg)
{
    HANDLE h = CreateFile(HEAP_STRING_AS_C_STRING(arg),0,0,NULL,
		   OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,INVALID_HANDLE_VALUE);

    if (h != INVALID_HANDLE_VALUE) {
        //
	DWORD lo;
	Val ml_lo;

	lo = GetFileSize(h,NULL);
	CloseHandle(h);
	if (lo != 0xffffffff) {
	    ml_lo =  make_one_word_unt(task,  lo  );
	    return  OPTION_THE( task, ml_lo );
	}
    }
    return OPTION_NULL;
}

#define REC_ALLOC8(task, r, a, b, c, d, e, f, g, h)	{	\
	Task	*__task = (task);				\
	Val	*__p = __task->heap_allocation_pointer;		\
	*__p++ = MAKE_TAGWORD(8, PAIRS_AND_RECORDS_BTAG);	\
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
    HANDLE h =  CreateFile(HEAP_STRING_AS_C_STRING(arg),0,0,NULL,
		   OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,INVALID_HANDLE_VALUE);

    if (h != INVALID_HANDLE_VALUE) {
	//
	FILETIME ft;

	if (GetFileTime(h,NULL,NULL,&ft)) {  /* request time of "last write" */
	    //
	    SYSTEMTIME st;

	    CloseHandle(h);

	    if (FileTimeToSystemTime(&ft,&st)) {

	      Val rec = make_eight_slot_rectord(task,
			    //
			    TAGGED_INT_FROM_C_INT((int)st.wYear),
			    TAGGED_INT_FROM_C_INT((int)st.wMonth),
			    TAGGED_INT_FROM_C_INT((int)st.wDayOfWeek),
			    TAGGED_INT_FROM_C_INT((int)st.wDay),
			    TAGGED_INT_FROM_C_INT((int)st.wHour),
			    TAGGED_INT_FROM_C_INT((int)st.wMinute),
			    TAGGED_INT_FROM_C_INT((int)st.wSecond),
			    TAGGED_INT_FROM_C_INT((int)st.wMilliseconds)
                        );

	      return OPTION_THE( task, rec );
	    }
	}
    }
    return OPTION_NULL;
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
    char name_buf[MAX_PATH];
    char path_buf[MAX_PATH];
    DWORD pblen;

    pblen = GetTempPath(MAX_PATH,path_buf);
    if ((pblen <= MAX_PATH) && 
	(GetTempFileName(path_buf,TMP_PREFIX,0,name_buf) != 0)) {

	Val tfn = make_ascii_string_from_c_string__may_heapclean(task,name_buf,NULL);

	return   OPTION_THE( task, tfn );
    }

    return   OPTION_NULL;
}

/* end of win32-file-system.c */


// COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.

