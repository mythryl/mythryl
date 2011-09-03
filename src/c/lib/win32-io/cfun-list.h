// cfun-list.h
//
//
// win32 C functions for IO
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"win32_io"
#define CLIB_VERSION	"0.2"
#define CLIB_DATE	"May 22, 1998"
#endif

CFUNC("get_std_handle","get_std_handle",      _lib7_win32_IO_get_std_handle,     "unt32->unt32")
CFUNC("set_file_pointer","set_file_pointer",      _lib7_win32_IO_set_file_pointer,     "(unt32*word32*unt32)->unt32")
CFUNC("read_vector","read_vector",      _lib7_win32_IO_read_vec,     "(unt32*int)->word8vector.vector")
CFUNC("read_rw_vector","read_rw_vector",      _lib7_win32_IO_read_arr,     "(unt32*word8array.Rw_Vector*int*int)->int")
CFUNC("read_vec_txt","read_vec_txt",      _lib7_win32_IO_read_vec_txt,     "(unt32*int)->char8vector.vector")
CFUNC("read_arr_txt","read_arr_txt",      _lib7_win32_IO_read_arr_txt,     "(unt32*char8array.Rw_Vector*int*int)->int")
CFUNC("close","close",      _lib7_win32_IO_close,     "unt32->Void")
CFUNC("create_file","create_file",      _lib7_win32_IO_create_file,     "(String*unt32*word32*unt32*word32)->unt32")
CFUNC("write_vector","write_vector",      _lib7_win32_IO_write_vec,     "(unt32*word8vector.vector*int*int)->int")
CFUNC("write_rw_vector","write_rw_vector",      _lib7_win32_IO_write_arr,     "(unt32*word8array.Rw_Vector*int*int)->int")
CFUNC("write_vec_txt","write_vec_txt",      _lib7_win32_IO_write_vec_txt,     "(unt32*word8vector.vector*int*int)->int")
CFUNC("write_arr_txt","write_arr_txt",      _lib7_win32_IO_write_arr_txt,     "(unt32*word8array.Rw_Vector*int*int)->int")
CFUNC("poll","poll", _lib7_win32_OS_poll,"unt32 list * (int1.Int * int) option -> unt32 list")


// COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

