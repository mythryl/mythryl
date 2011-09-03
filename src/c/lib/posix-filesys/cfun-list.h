// cfun-list.h
//
//
// This file lists the "posix_filesys" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  change_directory:  String -> Void
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "posix_filesys", fun_name => "chdir" };
// 
// or such -- see src/lib/std/src/posix-1003.1b/posix-file.pkg
// It gets #included by both:
//
//     src/c/lib/posix-filesys/libmythryl-posix-filesys.c
//     src/c/lib/posix-filesys/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"posix_filesys"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"February 16, 1995"
#endif

CFUNC("osval","osval",	   _lib7_P_FileSys_osval,      "String -> int")
CFUNC("chdir","chdir",     _lib7_P_FileSys_chdir,      "String -> Void")
CFUNC("getcwd","getcwd",    _lib7_P_FileSys_getcwd,     "Void -> String")
CFUNC("openf","openf",     _lib7_P_FileSys_openf,      "String * word * word -> int")
CFUNC("umask","umask",     _lib7_P_FileSys_umask,      "word -> word")
CFUNC("link","link",      _lib7_P_FileSys_link,       "String * String -> Void")
CFUNC("rename","rename",    _lib7_P_FileSys_rename,     "String * String -> Void")
CFUNC("symlink","symlink",   _lib7_P_FileSys_symlink,    "String * String -> Void")
CFUNC("mkdir","mkdir",     _lib7_P_FileSys_mkdir,      "String * word -> Void")
CFUNC("mkfifo","mkfifo",    _lib7_P_FileSys_mkfifo,     "String * word -> Void")
CFUNC("unlink","unlink",    _lib7_P_FileSys_unlink,     "String -> Void")
CFUNC("rmdir","rmdir",     _lib7_P_FileSys_rmdir,      "String -> Void")
CFUNC("readlink","readlink",  _lib7_P_FileSys_readlink,   "String -> String")
CFUNC("stat","stat",      _lib7_P_FileSys_stat,       "String -> statrep")
CFUNC("stat_64","stat_64",   _lib7_P_FileSys_stat_64,    "String -> statrep")
CFUNC("lstat","lstat",     _lib7_P_FileSys_lstat,      "String -> statrep")
CFUNC("lstat_64","lstat_64",  _lib7_P_FileSys_lstat_64,   "String -> statrep")
CFUNC("fstat","fstat",     _lib7_P_FileSys_fstat,      "word -> statrep")
CFUNC("fstat_64","fstat_64",  _lib7_P_FileSys_fstat_64,   "word -> statrep")
CFUNC("access","access",    _lib7_P_FileSys_access,     "String * word -> Bool")
CFUNC("chmod","chmod",     _lib7_P_FileSys_chmod,      "String * word -> Void")
CFUNC("fchmod","fchmod",    _lib7_P_FileSys_fchmod,     "int * word -> Void")
CFUNC("ftruncate","ftruncate", _lib7_P_FileSys_ftruncate,  "int * int -> Void")
CFUNC("ftruncate_64","ftruncate_64",_lib7_P_FileSys_ftruncate_64,"int * word2 * unt1 -> Void")
CFUNC("chown","chown",     _lib7_P_FileSys_chown,      "String * word * word -> Void")
CFUNC("fchown","fchown",    _lib7_P_FileSys_fchown,     "int * word * word -> Void")
CFUNC("utime","utime",     _lib7_P_FileSys_utime,      "String * int * int -> Void")
CFUNC("pathconf","pathconf",  _lib7_P_FileSys_pathconf,   "(String * String) -> word option")
CFUNC("fpathconf","fpathconf", _lib7_P_FileSys_fpathconf,  "(int * String) -> word option")
CFUNC("opendir","opendir",   _lib7_P_FileSys_opendir,    "String -> chunk")
CFUNC("readdir","readdir",   _lib7_P_FileSys_readdir,    "chunk -> String")
CFUNC("rewinddir","rewinddir", _lib7_P_FileSys_rewinddir,  "chunk -> Void")
CFUNC("closedir","closedir",  _lib7_P_FileSys_closedir,   "chunk -> Void")



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

