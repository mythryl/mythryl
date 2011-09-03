/* win32-io.c
 *
 * interface to win32 io functions
 */

#include "../../config.h"

#include <windows.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"

#include "win32-fault.h"

#define EOF_char           '\x01a'           /* ^Z is win32 eof */

/* macro to check if h is a console that hasn't been redirected */
#define IS_CONIN(h) (((h) == win32_stdin_handle) && \
		     (GetFileType(h) == FILE_TYPE_CHAR))

/* _lib7_win32_IO_get_std_handle: unt32 -> unt32
 * interface to win32 GetStdHandle
 */
Val _lib7_win32_IO_get_std_handle(Task *task, Val arg)
{
  Val_Sized_Unt w = WORD_LIB7toC(arg);
  HANDLE h = GetStdHandle(w);
  Val res;

#ifdef WIN32_DEBUG
  debug_say("getting std handle for %x as %x\n", w, (unsigned int) h);
#endif
  WORD_ALLOC(task, res, (Val_Sized_Unt)h);
  return res;
}

/* _lib7_win32_IO_close: unt32 -> Void
 * close a handle
 */
Val _lib7_win32_IO_close(Task *task, Val arg)
{
    HANDLE h = (HANDLE) WORD_LIB7toC(arg);

    if (CloseHandle(h)) {
      return HEAP_VOID;
    }

#ifdef WIN32_DEBUG
    debug_say("_lib7_win32_IO_close: failing\n");
#endif

    return RAISE_SYSERR(task,-1);
}


/* _lib7_win32_IO_set_file_pointer: (unt32 * unt32 * unt32) -> unt32
 *                                 handle   dist     how
 */
Val _lib7_win32_IO_set_file_pointer(Task *task, Val arg)
{
  HANDLE h = (HANDLE) WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg,0));
  LONG dist = (LONG) WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg,1));
  DWORD how = (DWORD) WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg,2));
  Val_Sized_Unt w;
  Val res;

  w = SetFilePointer(h,dist,NULL,how);
  WORD_ALLOC(task, res, w);
  return res;
}

/* remove CRs ('\r') from buf of size *np; sets *np to be the new buf size 
 */
static rm_CRs(char *buf,int *np)
{
  int i, j = 0;
  int n = *np;

  for (i = 0; i < n; i++) {
    if (buf[i] != '\r') {
      buf[j++] = buf[i];
    }
  }
  *np = j;
}


/* translate CRs ('\r') to newlines ('\n'), removing existing LFs (also '\n').
 * process backspace (BS)
 * sets *np to the new buffer size
 * returns TRUE if the buffer contains the EOF character
 */
static Bool CRLF_EOFscan(char *buf,int *np)
{
  int i, j = 0;
  int n = *np;
  Bool sawEOF = FALSE;

  for (i = 0; i<n; i++) {
    if (buf[i] == '\r') {             /* translate CRs */
      buf[j++] = '\n';
    } else if (buf[i] == '\b') {      /* process BSes */
      if (j) j--;
    } else if (buf[i] != '\n') {
      if (buf[i] == EOF_char)
	sawEOF = TRUE;
      buf[j++] = buf[i];
    }
  }
  *np = j;
  return sawEOF;
}

/* _lib7_win32_IO_read_vec : (unt32 * int) -> word8vector.Vector
 *                          handle   nbytes
 *
 * Read the specified number of bytes from the specified handle,
 * returning them in a vector.
 *
 * Note: Read operations on console devices do not trap ctrl-C.
 *       ctrl-Cs are placed in the input buffer.
 */
Val _lib7_win32_IO_read_vec(Task *task, Val arg)
{
    HANDLE h = (HANDLE) WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg, 0));
    DWORD nbytes = (DWORD) GET_TUPLE_SLOT_AS_INT(arg, 1);
    DWORD n;

    // Allocate the vector.
    // Note that this might cause a GC:
    //
    Val vec = allocate_nonempty_int1_vector( task, BYTES_TO_WORDS (nbytes) );

    if (ReadFile( h, PTR_CAST(void*, vec), nbytes, &n, NULL)) {

        if (n == 0) {
#ifdef WIN32_DEBUG
            debug_say("_lib7_win32_IO_read_vec: eof on device\n");
#endif
            return ZERO_LENGTH_STRING_GLOBAL;
        }

        if (n < nbytes) {
	    //
            shrink_fresh_int1_vector( task, vec, BYTES_TO_WORDS(n) );
        }

        /* Allocate header: */
        {   Val result;
            SEQHDR_ALLOC (task, result, STRING_TAGWORD, vec, n);
            return result;
        }

    } else {
#ifdef WIN32_DEBUG
        debug_say("_lib7_win32_IO_read_vec: failing %d %d\n",n,nbytes);
#endif
        return RAISE_SYSERR(task,-1);
    }
}

static void check_cntrl_c(BOOL read_OK,int bytes_read)
{
  /* this is a rude hack */
  /* under NT and default console mode, on 
   *  EOF: read_OK is true, and n > 0
   *   ^C: read_OK is true, and n == 0.  However, the cntrl_c handler is
   *       not always invoked before ReadConsole returns.
   */
  /* under 95 and default console mode, on
   *  EOF: read_OK is true and n is 0
   *   ^C: read_OK is true, n is 0, but handler seems to always have been run
   */
  if (read_OK &&  
      (bytes_read == 0) && 
      win32_isNT) {
    /* guaranteed that a cntrl_c has occurred and has not been reset */
    /* wait for it to happen */
    wait_for_cntrl_c();
  }
}

/* _lib7_win32_IO_read_vec_txt : (unt32 * int) -> char8vector.Vector
 *                             handle   nbytes
 *
 * Read the specified number of bytes from the specified handle,
 * returning them in a vector.
 *
 * reflect changes in _lib7_win32_IO_read_arr_txt
 */
Val _lib7_win32_IO_read_vec_txt(Task *task, Val arg)
{
  HANDLE h = (HANDLE) WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg, 0));
  DWORD nbytes = (DWORD) GET_TUPLE_SLOT_AS_INT(arg, 1);
  Val vec, res;
  DWORD	n;
  BOOL flag = FALSE;

  // Allocate the vector.
  / Note that this might cause a GC.
  //
  vec = allocate_nonempty_int1_vector( task, BYTES_TO_WORDS (nbytes) );

  if (IS_CONIN(h)) {
    flag = ReadConsole(h,PTR_CAST(void*,vec),nbytes,&n,NULL);
    check_cntrl_c(flag,n); 
  } else {
    flag = ReadFile(h,PTR_CAST(void*,vec),nbytes,&n,NULL);
  }
  if (flag) {
    if (IS_CONIN(h)) {
      if (CRLF_EOFscan((char *)vec,&n)) {
	n = 0;
      }
    } 
    else {
      rm_CRs((char *)vec,&n);
    }

    if (n == 0) {
#ifdef WIN32_DEBUG
      debug_say("_lib7_win32_IO_read_vec_txt: eof on device\n");
#endif
      return ZERO_LENGTH_STRING_GLOBAL;
    }
    if (n < nbytes) {
        shrink_fresh_int1_vector( task, vec, BYTES_TO_WORDS(n) );
    }
    // Allocate header:
    SEQHDR_ALLOC (task, res, STRING_TAGWORD, vec, n);
#ifdef WIN32_DEBUG
    debug_say("_lib7_win32_IO_read_vec_txt: read %d\n",n);
#endif
    return res;
  }
  else if ((h == win32_stdin_handle) &&             /* input from stdin */
	   (GetFileType(h) == FILE_TYPE_PIPE) &&    /* but not console */
	   (GetLastError() == ERROR_BROKEN_PIPE)) { /* and pipe broken */
    /* this is an EOF on redirected stdin (ReadFile failed) */
    return ZERO_LENGTH_STRING_GLOBAL;
  }
  else {
#ifdef WIN32_DEBUG
    debug_say("_lib7_win32_IO_read_vec_txt: failing on handle %x\n",h);
#endif
    return RAISE_SYSERR(task,-1);
  }
}

/* _lib7_win32_IO_read_arr : (unt32*word8array.Rw_Vector*int*int) -> int
 *                          handle buffer           n   start
 *
 * Read n bytes of data from the specified handle into the given array, 
 * starting at start. Return the number of bytes read. Assume bounds
 * have been checked.
 *
 * Note: Read operations on console devices do not trap ctrl-C.
 *       ctrl-Cs are placed in the input buffer.
 */
Val _lib7_win32_IO_read_arr(Task *task, Val arg)
{
  HANDLE h = (HANDLE) WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg, 0));
  Val buf = GET_TUPLE_SLOT_AS_VAL(arg,1);
  DWORD nbytes = (DWORD) GET_TUPLE_SLOT_AS_INT(arg, 2);
  Unt8 *start = HEAP_STRING_AS_C_STRING(buf) + GET_TUPLE_SLOT_AS_INT(arg,3);
  DWORD n;

  if (ReadFile(h,PTR_CAST(void*,start),nbytes,&n,NULL)) {
#ifdef WIN32_DEBUG
    if (n == 0)
      debug_say("_lib7_win32_IO_read_arr: eof on device\n");
#endif
    return TAGGED_INT_FROM_C_INT(n);
  } 
#ifdef WIN32_DEBUG
  debug_say("_lib7_win32_IO_read_arr: failing\n");
#endif
  return RAISE_SYSERR(task,-1);
}

/* _lib7_win32_IO_read_arr_txt : (unt32*char8array.Rw_Vector*int*int) -> int
 *                              handle buffer           n   start
 *
 * Read n bytes of data from the specified handle into the given array, 
 * starting at start. Return the number of bytes read. Assume bounds
 * have been checked.
 *
 * reflect changes in _lib7_win32_IO_read_vec_txt
 */
Val _lib7_win32_IO_read_arr_txt(Task *task, Val arg)
{
  HANDLE h = (HANDLE) WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg, 0));
  Val buf = GET_TUPLE_SLOT_AS_VAL(arg,1);
  DWORD	nbytes = (DWORD) GET_TUPLE_SLOT_AS_INT(arg, 2);
  Unt8 *start = HEAP_STRING_AS_C_STRING(buf) + GET_TUPLE_SLOT_AS_INT(arg,3);
  DWORD	n;
  BOOL flag;

  if (IS_CONIN(h)) {
    flag = ReadConsole(h,PTR_CAST(void*,start),nbytes,&n,NULL);
    check_cntrl_c(flag,n); 
  } else {
    flag = ReadFile(h,PTR_CAST(void*,start),nbytes,&n,NULL);
  }
  if (flag) {
    if (IS_CONIN(h)) {
      if (CRLF_EOFscan((char *)start,&n)) {
	n = 0;
      }
    } 
    else {
      rm_CRs((char *)buf,&n);
    }
#ifdef WIN32_DEBUG
    debug_say("_lib7_win32_IO_read_arr_txt: eof on device\n");
#endif
    return TAGGED_INT_FROM_C_INT(n);
  } else {
    if ((h == win32_stdin_handle) &&             /* input from stdin */
	(GetFileType(h) == FILE_TYPE_PIPE) &&    /* but not console */
        (GetLastError() == ERROR_BROKEN_PIPE)) { /* and pipe broken */
      /* this is an EOF on redirected stdin (ReadFile failed) */
      return TAGGED_INT_FROM_C_INT(0);
    } 
  }
#ifdef WIN32_DEBUG
  debug_say("_lib7_win32_IO_read_arr_txt: failing\n");
#endif
  return RAISE_SYSERR(task,-1);
}


/* _lib7_win32_IO_create_file: (String*unt32*word32*unt32*word32) -> unt32 
 *                            name   access share  create attribute       handle
 *
 * create file "name" with access, share, create, and attribute flags
 */
Val _lib7_win32_IO_create_file(Task *task, Val arg)
{
  Val fname = GET_TUPLE_SLOT_AS_VAL(arg,0);
  char *name = HEAP_STRING_AS_C_STRING(fname);
  DWORD access = WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg,1));
  DWORD share = WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg,2));
  DWORD create = WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg,3));
  DWORD attribute = WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg,4));
  HANDLE h =  CreateFile(name,access,share,NULL,create,attribute,INVALID_HANDLE_VALUE);
  Val res;

#ifdef WIN32_DEBUG
  if (h == INVALID_HANDLE_VALUE)
    debug_say("_lib7_win32_IO_create_file: failing\n");
#endif
  WORD_ALLOC(task, res, (Val_Sized_Unt)h);
  return res;
}

/* _lib7_win32_IO_write_buf : (unt32*word8vector.Vector*int*int) -> int
 *                           handle buf                n   offset
 *
 * generic routine for writing n byes from buf to handle starting at offset
 *
 */
Val _lib7_win32_IO_write_buf(Task *task, Val arg)
{
  HANDLE h = (HANDLE) WORD_LIB7toC(GET_TUPLE_SLOT_AS_VAL(arg,0));
  Val buf = GET_TUPLE_SLOT_AS_VAL(arg,1);
  size_t nbytes = GET_TUPLE_SLOT_AS_INT(arg,2);
  Unt8 *start = (Unt8 *) (HEAP_STRING_AS_C_STRING(buf) + GET_TUPLE_SLOT_AS_INT(arg, 3));
  DWORD n;

#ifdef WIN32_DEBUG
  debug_say("_lib7_win32_IO_write_buf: handle is %x\n", (unsigned int) h);
#endif
  if (WriteFile(h,PTR_CAST(void*,start),nbytes,&n,NULL)) {
#ifdef WIN32_DEBUG
    if (n == 0)
      debug_say("_lib7_win32_IO_write_buf: eof on device\n");
#endif
    return TAGGED_INT_FROM_C_INT(n);
  }
#ifdef WIN32_DEBUG
  debug_say("_lib7_win32_IO_write_buf: failing\n");
#endif
  return RAISE_SYSERR(task,-1);
}

Val _lib7_win32_IO_write_vec(Task *task, Val arg)
{ 
  return _lib7_win32_IO_write_buf(task,arg);
}

Val _lib7_win32_IO_write_arr(Task *task, Val arg)
{ 
  return _lib7_win32_IO_write_buf(task,arg);
}

Val _lib7_win32_IO_write_vec_txt(Task *task, Val arg)
{ 
  return _lib7_win32_IO_write_buf(task,arg);
}

Val _lib7_win32_IO_write_arr_txt(Task *task, Val arg)
{ 
  return _lib7_win32_IO_write_buf(task,arg);
}

/* end of win32-io.c */


/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
 * released under Gnu Public Licence version 3.
 */

