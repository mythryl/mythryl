/* win32-io.c
 *
 * interface to win32 io functions
 */

#include "../../config.h"

#include <windows.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"

#include "win32-fault.h"

#define EOF_char           '\x01a'           /* ^Z is win32 eof */

/* macro to check if h is a console that hasn't been redirected */
#define IS_CONIN(h) (((h) == win32_stdin_handle) && \
		     (GetFileType(h) == FILE_TYPE_CHAR))

/* _lib7_win32_IO_get_std_handle: word32 -> word32
 * interface to win32 GetStdHandle
 */
lib7_val_t _lib7_win32_IO_get_std_handle(lib7_state_t *lib7_state, lib7_val_t arg)
{
  Word_t w = WORD_LIB7toC(arg);
  HANDLE h = GetStdHandle(w);
  lib7_val_t res;

#ifdef WIN32_DEBUG
  SayDebug("getting std handle for %x as %x\n", w, (unsigned int) h);
#endif
  WORD_ALLOC(lib7_state, res, (Word_t)h);
  return res;
}

/* _lib7_win32_IO_close: word32 -> Void
 * close a handle
 */
lib7_val_t _lib7_win32_IO_close(lib7_state_t *lib7_state, lib7_val_t arg)
{
  HANDLE h = (HANDLE) WORD_LIB7toC(arg);
  
  if (CloseHandle(h)) {
    return LIB7_void;
  }
#ifdef WIN32_DEBUG
  SayDebug("_lib7_win32_IO_close: failing\n");
#endif
  return RAISE_SYSERR(lib7_state,-1);
}


/* _lib7_win32_IO_set_file_pointer: (word32 * word32 * word32) -> word32
 *                                 handle   dist     how
 */
lib7_val_t _lib7_win32_IO_set_file_pointer(lib7_state_t *lib7_state, lib7_val_t arg)
{
  HANDLE h = (HANDLE) WORD_LIB7toC(REC_SEL(arg,0));
  LONG dist = (LONG) WORD_LIB7toC(REC_SEL(arg,1));
  DWORD how = (DWORD) WORD_LIB7toC(REC_SEL(arg,2));
  Word_t w;
  lib7_val_t res;

  w = SetFilePointer(h,dist,NULL,how);
  WORD_ALLOC(lib7_state, res, w);
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
static bool_t CRLF_EOFscan(char *buf,int *np)
{
  int i, j = 0;
  int n = *np;
  bool_t sawEOF = FALSE;

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

/* _lib7_win32_IO_read_vec : (word32 * int) -> word8vector.Vector
 *                          handle   nbytes
 *
 * Read the specified number of bytes from the specified handle,
 * returning them in a vector.
 *
 * Note: Read operations on console devices do not trap ctrl-C.
 *       ctrl-Cs are placed in the input buffer.
 */
lib7_val_t _lib7_win32_IO_read_vec(lib7_state_t *lib7_state, lib7_val_t arg)
{
    HANDLE h = (HANDLE) WORD_LIB7toC(REC_SEL(arg, 0));
    DWORD nbytes = (DWORD) REC_SELINT(arg, 1);
    DWORD n;

    /* Allocate the vector.
     * Note that this might cause a GC:
     */
    lib7_val_t vec = LIB7_AllocRaw32 (lib7_state, BYTES_TO_WORDS (nbytes));

    if (ReadFile( h, PTR_LIB7toC(void, vec), nbytes, &n, NULL)) {

        if (n == 0) {
#ifdef WIN32_DEBUG
            SayDebug("_lib7_win32_IO_read_vec: eof on device\n");
#endif
            return LIB7_string0;
        }

        if (n < nbytes) {
            /* We need to shrink the vector: */
            LIB7_ShrinkRaw32 (lib7_state, vec, BYTES_TO_WORDS(n));
        }

        /* Allocate header: */
        {   lib7_val_t result;
            SEQHDR_ALLOC (lib7_state, result, DESC_string, vec, n);
            return result;
        }

    } else {
#ifdef WIN32_DEBUG
        SayDebug("_lib7_win32_IO_read_vec: failing %d %d\n",n,nbytes);
#endif
        return RAISE_SYSERR(lib7_state,-1);
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

/* _lib7_win32_IO_read_vec_txt : (word32 * int) -> char8vector.Vector
 *                             handle   nbytes
 *
 * Read the specified number of bytes from the specified handle,
 * returning them in a vector.
 *
 * reflect changes in _lib7_win32_IO_read_arr_txt
 */
lib7_val_t _lib7_win32_IO_read_vec_txt(lib7_state_t *lib7_state, lib7_val_t arg)
{
  HANDLE h = (HANDLE) WORD_LIB7toC(REC_SEL(arg, 0));
  DWORD nbytes = (DWORD) REC_SELINT(arg, 1);
  lib7_val_t vec, res;
  DWORD	n;
  BOOL flag = FALSE;

  /* Allocate the vector.
   * Note that this might cause a GC.
   */
  vec = LIB7_AllocRaw32 (lib7_state, BYTES_TO_WORDS (nbytes));

  if (IS_CONIN(h)) {
    flag = ReadConsole(h,PTR_LIB7toC(void,vec),nbytes,&n,NULL);
    check_cntrl_c(flag,n); 
  } else {
    flag = ReadFile(h,PTR_LIB7toC(void,vec),nbytes,&n,NULL);
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
      SayDebug("_lib7_win32_IO_read_vec_txt: eof on device\n");
#endif
      return LIB7_string0;
    }
    if (n < nbytes) {
      /* shrink buffer */
      LIB7_ShrinkRaw32 (lib7_state, vec, BYTES_TO_WORDS(n));
    }
    /* allocate header */
    SEQHDR_ALLOC (lib7_state, res, DESC_string, vec, n);
#ifdef WIN32_DEBUG
    SayDebug("_lib7_win32_IO_read_vec_txt: read %d\n",n);
#endif
    return res;
  }
  else if ((h == win32_stdin_handle) &&             /* input from stdin */
	   (GetFileType(h) == FILE_TYPE_PIPE) &&    /* but not console */
	   (GetLastError() == ERROR_BROKEN_PIPE)) { /* and pipe broken */
    /* this is an EOF on redirected stdin (ReadFile failed) */
    return LIB7_string0;
  }
  else {
#ifdef WIN32_DEBUG
    SayDebug("_lib7_win32_IO_read_vec_txt: failing on handle %x\n",h);
#endif
    return RAISE_SYSERR(lib7_state,-1);
  }
}

/* _lib7_win32_IO_read_arr : (word32*word8array.Rw_Vector*int*int) -> int
 *                          handle buffer           n   start
 *
 * Read n bytes of data from the specified handle into the given array, 
 * starting at start. Return the number of bytes read. Assume bounds
 * have been checked.
 *
 * Note: Read operations on console devices do not trap ctrl-C.
 *       ctrl-Cs are placed in the input buffer.
 */
lib7_val_t _lib7_win32_IO_read_arr(lib7_state_t *lib7_state, lib7_val_t arg)
{
  HANDLE h = (HANDLE) WORD_LIB7toC(REC_SEL(arg, 0));
  lib7_val_t buf = REC_SEL(arg,1);
  DWORD nbytes = (DWORD) REC_SELINT(arg, 2);
  Byte_t *start = STR_LIB7toC(buf) + REC_SELINT(arg,3);
  DWORD n;

  if (ReadFile(h,PTR_LIB7toC(void,start),nbytes,&n,NULL)) {
#ifdef WIN32_DEBUG
    if (n == 0)
      SayDebug("_lib7_win32_IO_read_arr: eof on device\n");
#endif
    return INT_CtoLib7(n);
  } 
#ifdef WIN32_DEBUG
  SayDebug("_lib7_win32_IO_read_arr: failing\n");
#endif
  return RAISE_SYSERR(lib7_state,-1);
}

/* _lib7_win32_IO_read_arr_txt : (word32*char8array.Rw_Vector*int*int) -> int
 *                              handle buffer           n   start
 *
 * Read n bytes of data from the specified handle into the given array, 
 * starting at start. Return the number of bytes read. Assume bounds
 * have been checked.
 *
 * reflect changes in _lib7_win32_IO_read_vec_txt
 */
lib7_val_t _lib7_win32_IO_read_arr_txt(lib7_state_t *lib7_state, lib7_val_t arg)
{
  HANDLE h = (HANDLE) WORD_LIB7toC(REC_SEL(arg, 0));
  lib7_val_t buf = REC_SEL(arg,1);
  DWORD	nbytes = (DWORD) REC_SELINT(arg, 2);
  Byte_t *start = STR_LIB7toC(buf) + REC_SELINT(arg,3);
  DWORD	n;
  BOOL flag;

  if (IS_CONIN(h)) {
    flag = ReadConsole(h,PTR_LIB7toC(void,start),nbytes,&n,NULL);
    check_cntrl_c(flag,n); 
  } else {
    flag = ReadFile(h,PTR_LIB7toC(void,start),nbytes,&n,NULL);
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
    SayDebug("_lib7_win32_IO_read_arr_txt: eof on device\n");
#endif
    return INT_CtoLib7(n);
  } else {
    if ((h == win32_stdin_handle) &&             /* input from stdin */
	(GetFileType(h) == FILE_TYPE_PIPE) &&    /* but not console */
        (GetLastError() == ERROR_BROKEN_PIPE)) { /* and pipe broken */
      /* this is an EOF on redirected stdin (ReadFile failed) */
      return INT_CtoLib7(0);
    } 
  }
#ifdef WIN32_DEBUG
  SayDebug("_lib7_win32_IO_read_arr_txt: failing\n");
#endif
  return RAISE_SYSERR(lib7_state,-1);
}


/* _lib7_win32_IO_create_file: (String*word32*word32*word32*word32) -> word32 
 *                            name   access share  create attribute       handle
 *
 * create file "name" with access, share, create, and attribute flags
 */
lib7_val_t _lib7_win32_IO_create_file(lib7_state_t *lib7_state, lib7_val_t arg)
{
  lib7_val_t fname = REC_SEL(arg,0);
  char *name = STR_LIB7toC(fname);
  DWORD access = WORD_LIB7toC(REC_SEL(arg,1));
  DWORD share = WORD_LIB7toC(REC_SEL(arg,2));
  DWORD create = WORD_LIB7toC(REC_SEL(arg,3));
  DWORD attribute = WORD_LIB7toC(REC_SEL(arg,4));
  HANDLE h =  CreateFile(name,access,share,NULL,create,attribute,INVALID_HANDLE_VALUE);
  lib7_val_t res;

#ifdef WIN32_DEBUG
  if (h == INVALID_HANDLE_VALUE)
    SayDebug("_lib7_win32_IO_create_file: failing\n");
#endif
  WORD_ALLOC(lib7_state, res, (Word_t)h);
  return res;
}

/* _lib7_win32_IO_write_buf : (word32*word8vector.Vector*int*int) -> int
 *                           handle buf                n   offset
 *
 * generic routine for writing n byes from buf to handle starting at offset
 *
 */
lib7_val_t _lib7_win32_IO_write_buf(lib7_state_t *lib7_state, lib7_val_t arg)
{
  HANDLE h = (HANDLE) WORD_LIB7toC(REC_SEL(arg,0));
  lib7_val_t buf = REC_SEL(arg,1);
  size_t nbytes = REC_SELINT(arg,2);
  Byte_t *start = (Byte_t *) (STR_LIB7toC(buf) + REC_SELINT(arg, 3));
  DWORD n;

#ifdef WIN32_DEBUG
  SayDebug("_lib7_win32_IO_write_buf: handle is %x\n", (unsigned int) h);
#endif
  if (WriteFile(h,PTR_LIB7toC(void,start),nbytes,&n,NULL)) {
#ifdef WIN32_DEBUG
    if (n == 0)
      SayDebug("_lib7_win32_IO_write_buf: eof on device\n");
#endif
    return INT_CtoLib7(n);
  }
#ifdef WIN32_DEBUG
  SayDebug("_lib7_win32_IO_write_buf: failing\n");
#endif
  return RAISE_SYSERR(lib7_state,-1);
}

lib7_val_t _lib7_win32_IO_write_vec(lib7_state_t *lib7_state, lib7_val_t arg)
{ 
  return _lib7_win32_IO_write_buf(lib7_state,arg);
}

lib7_val_t _lib7_win32_IO_write_arr(lib7_state_t *lib7_state, lib7_val_t arg)
{ 
  return _lib7_win32_IO_write_buf(lib7_state,arg);
}

lib7_val_t _lib7_win32_IO_write_vec_txt(lib7_state_t *lib7_state, lib7_val_t arg)
{ 
  return _lib7_win32_IO_write_buf(lib7_state,arg);
}

lib7_val_t _lib7_win32_IO_write_arr_txt(lib7_state_t *lib7_state, lib7_val_t arg)
{ 
  return _lib7_win32_IO_write_buf(lib7_state,arg);
}

/* end of win32-io.c */


/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

