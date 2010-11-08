/* cfun-list.h
 *
 *
 * win32 C functions for IO
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"WIN32-IO"
#define CLIB_VERSION	"0.2"
#define CLIB_DATE	"May 22, 1998"
#endif

CFUNC("get_std_handle",\
      _lib7_win32_IO_get_std_handle,\
     "word32->word32")
CFUNC("set_file_pointer",\
      _lib7_win32_IO_set_file_pointer,\
     "(word32*word32*word32)->word32")
CFUNC("read_vector",\
      _lib7_win32_IO_read_vec,\
     "(word32*int)->word8vector.vector")
CFUNC("read_rw_vector",\
      _lib7_win32_IO_read_arr,\
     "(word32*word8array.Rw_Vector*int*int)->int")
CFUNC("read_vec_txt",\
      _lib7_win32_IO_read_vec_txt,\
     "(word32*int)->char8vector.vector")
CFUNC("read_arr_txt",\
      _lib7_win32_IO_read_arr_txt,\
     "(word32*char8array.Rw_Vector*int*int)->int")
CFUNC("close",\
      _lib7_win32_IO_close,\
     "word32->Void")
CFUNC("create_file",\
      _lib7_win32_IO_create_file,\
     "(String*word32*word32*word32*word32)->word32")
CFUNC("write_vector",\
      _lib7_win32_IO_write_vec,\
     "(word32*word8vector.vector*int*int)->int")
CFUNC("write_rw_vector",\
      _lib7_win32_IO_write_arr,\
     "(word32*word8array.Rw_Vector*int*int)->int")
CFUNC("write_vec_txt",\
      _lib7_win32_IO_write_vec_txt,\
     "(word32*word8vector.vector*int*int)->int")
CFUNC("write_arr_txt",\
      _lib7_win32_IO_write_arr_txt,\
     "(word32*word8array.Rw_Vector*int*int)->int")

CFUNC("poll", _lib7_win32_OS_poll,"word32 list * (int32.Int * int) option -> word32 list")


/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
