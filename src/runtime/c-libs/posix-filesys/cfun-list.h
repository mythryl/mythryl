/* cfun-list.h
 *
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"POSIX-FileSys"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"February 16, 1995"
#endif

CFUNC("osval",	   _lib7_P_FileSys_osval,      "String -> int")
CFUNC("chdir",     _lib7_P_FileSys_chdir,      "String -> Void")
CFUNC("getcwd",    _lib7_P_FileSys_getcwd,     "Void -> String")
CFUNC("openf",     _lib7_P_FileSys_openf,      "String * word * word -> int")
CFUNC("umask",     _lib7_P_FileSys_umask,      "word -> word")
CFUNC("link",      _lib7_P_FileSys_link,       "String * String -> Void")
CFUNC("rename",    _lib7_P_FileSys_rename,     "String * String -> Void")
CFUNC("symlink",   _lib7_P_FileSys_symlink,    "String * String -> Void")
CFUNC("mkdir",     _lib7_P_FileSys_mkdir,      "String * word -> Void")
CFUNC("mkfifo",    _lib7_P_FileSys_mkfifo,     "String * word -> Void")
CFUNC("unlink",    _lib7_P_FileSys_unlink,     "String -> Void")
CFUNC("rmdir",     _lib7_P_FileSys_rmdir,      "String -> Void")
CFUNC("readlink",  _lib7_P_FileSys_readlink,   "String -> String")
CFUNC("stat",      _lib7_P_FileSys_stat,       "String -> statrep")
CFUNC("stat_64",   _lib7_P_FileSys_stat_64,    "String -> statrep")
CFUNC("lstat",     _lib7_P_FileSys_lstat,      "String -> statrep")
CFUNC("lstat_64",  _lib7_P_FileSys_lstat_64,   "String -> statrep")
CFUNC("fstat",     _lib7_P_FileSys_fstat,      "word -> statrep")
CFUNC("fstat_64",  _lib7_P_FileSys_fstat_64,   "word -> statrep")
CFUNC("access",    _lib7_P_FileSys_access,     "String * word -> Bool")
CFUNC("chmod",     _lib7_P_FileSys_chmod,      "String * word -> Void")
CFUNC("fchmod",    _lib7_P_FileSys_fchmod,     "int * word -> Void")
CFUNC("ftruncate", _lib7_P_FileSys_ftruncate,  "int * int -> Void")
CFUNC("ftruncate_64",_lib7_P_FileSys_ftruncate_64,"int * word2 * word32 -> Void")
CFUNC("chown",     _lib7_P_FileSys_chown,      "String * word * word -> Void")
CFUNC("fchown",    _lib7_P_FileSys_fchown,     "int * word * word -> Void")
CFUNC("utime",     _lib7_P_FileSys_utime,      "String * int * int -> Void")
CFUNC("pathconf",  _lib7_P_FileSys_pathconf,   "(String * String) -> word option")
CFUNC("fpathconf", _lib7_P_FileSys_fpathconf,  "(int * String) -> word option")
CFUNC("opendir",   _lib7_P_FileSys_opendir,    "String -> chunk")
CFUNC("readdir",   _lib7_P_FileSys_readdir,    "chunk -> String")
CFUNC("rewinddir", _lib7_P_FileSys_rewinddir,  "chunk -> Void")
CFUNC("closedir",  _lib7_P_FileSys_closedir,   "chunk -> Void")



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
