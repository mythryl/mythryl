/* cfun-list.h
 *
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"POSIX-IO"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"February 16, 1995"
#endif

CFUNC("osval",	    _lib7_P_IO_osval,     "String -> int")
CFUNC("pipe",       _lib7_P_IO_pipe,      "Void -> int * int")
CFUNC("dup",        _lib7_P_IO_dup,       "int -> int")
CFUNC("dup2",       _lib7_P_IO_dup2,      "int * int -> Void")
CFUNC("close",      _lib7_P_IO_close,     "int -> Void")
CFUNC("read",       _lib7_P_IO_read,      "int * int -> unt8_vector.Vector")
CFUNC("readbuf",    _lib7_P_IO_readbuf,   "int * rw_unt8_vector.Rw_Vector * int -> int")
CFUNC("write",      _lib7_P_IO_write,     "int * unt8_vector.Vector * int -> int")
CFUNC("writebuf",   _lib7_P_IO_writebuf,  "int * rw_unt8_vector.Rw_Vector * int * int -> int")
CFUNC("fcntl_d",    _lib7_P_IO_fcntl_d,   "int * int -> int")
CFUNC("fcntl_gfd",  _lib7_P_IO_fcntl_gfd, "int -> word")
CFUNC("fcntl_sfd",  _lib7_P_IO_fcntl_sfd, "int * word -> Void")
CFUNC("fcntl_gfl",  _lib7_P_IO_fcntl_gfl, "int -> word * word")
CFUNC("fcntl_sfl",  _lib7_P_IO_fcntl_sfl, "int * word -> Void")
CFUNC("fcntl_l",    _lib7_P_IO_fcntl_l,   "int * int * flock_rep -> flock_rep")
CFUNC("fcntl_l_64", _lib7_P_IO_fcntl_l_64,"int * int * flock_rep -> flock_rep")
CFUNC("lseek",      _lib7_P_IO_lseek,     "int * int * int -> int")
CFUNC("lseek_64",   _lib7_P_IO_lseek_64,  "int * int * offset -> offset")
CFUNC("fsync",      _lib7_P_IO_fsync,     "int -> Void")


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
