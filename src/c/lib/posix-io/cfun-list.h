// cfun-list.h
//
//
// This file lists the "posix_io" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  dup2:  (Sy_Int, Sy_Int) -> Void
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "posix_io", fun_name => "dup2" };
// 
// or such -- see src/lib/std/src/psx/posix-io.pkg
// It gets #included by both:
//
//     src/c/lib/posix-io/libmythryl-posix-io.c
//     src/c/lib/posix-io/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"posix_io"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"February 16, 1995"
#endif

CFUNC("osval","osval",	    _lib7_P_IO_osval,     "String -> Int")
CFUNC("pipe","pipe",       _lib7_P_IO_pipe,      "Void -> (Int, Int)")
CFUNC("close","close",      _lib7_P_IO_close,     "Int -> Void")
CFUNC("copy","copy",      _lib7_P_IO_copy,     "(String, String) -> Int")
CFUNC("dup","dup",        _lib7_P_IO_dup,       "Int -> Int")
CFUNC("dup2","dup2",       _lib7_P_IO_dup2,      "(Int, Int) -> Void")
CFUNC("equal","equal",      _lib7_P_IO_equal,     "(String, String) -> Bool")
CFUNC("read","read",       _lib7_P_IO_read,      "(Int, Int) -> vector_of_one_byte_unts::Vector")
CFUNC("readbuf","readbuf",    _lib7_P_IO_readbuf,   "(Int, rw_vector_of_one_byte_unts::Rw_Vector, Int) -> Int")
CFUNC("write","write",      _lib7_P_IO_write,     "(Int, vector_of_one_byte_unts::Vector, Int) -> Int")
CFUNC("writebuf","writebuf",   _lib7_P_IO_writebuf,  "(Int, rw_vector_of_one_byte_unts::Rw_Vector, Int, Int -> Int")
CFUNC("fcntl_d","fcntl_d",    _lib7_P_IO_fcntl_d,   "(Int, Int) -> Int")
CFUNC("fcntl_gfd","fcntl_gfd",  _lib7_P_IO_fcntl_gfd, "Int -> Unt")
CFUNC("fcntl_sfd","fcntl_sfd",  _lib7_P_IO_fcntl_sfd, "(Int, Unt) -> Void")
CFUNC("fcntl_gfl","fcntl_gfl",  _lib7_P_IO_fcntl_gfl, "Int -> (Unt, Unt)")
CFUNC("fcntl_sfl","fcntl_sfl",  _lib7_P_IO_fcntl_sfl, "(Int, Unt) -> Void")
CFUNC("fcntl_l","fcntl_l",    _lib7_P_IO_fcntl_l,   "(Int, Int, flock_rep) -> flock_rep")
CFUNC("fcntl_l_64","fcntl_l_64", _lib7_P_IO_fcntl_l_64,"(Int, Int, flock_rep) -> flock_rep")
CFUNC("lseek","lseek",      _lib7_P_IO_lseek,     "(Int, Int, Int) -> Int")
CFUNC("lseek_64","lseek_64",   _lib7_P_IO_lseek_64,  "(Int, Int, offset) -> offset")
CFUNC("fsync","fsync",      _lib7_P_IO_fsync,     "Int -> Void")


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

