// cfun-list.h
//
//
// This file lists the "posix_passwd_db" library of Mythryl-callable
// C functions, accessible at the Mythryl level via:
//
//     my  getgrgid:  Unt -> (String, Unt, List(String))
//         =
//	   mythryl_callable_c_library_interface::find_c_function { lib_name => "posix_passwd_db", fun_name => "getgrgid" };
// 
// or such -- see src/lib/std/src/posix-1003.1b/posix-etc.pkg
// It gets #included by both:
//
//     src/c/lib/posix-passwd/libmythryl-posix-passwd-db.c
//     src/c/lib/posix-passwd/cfun-proto-list.h
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"posix_passwd_db"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"February 16, 1995"
#endif

CFUNC("getgrgid","getgrgid",  _lib7_P_SysDB_getgrgid,  "Unt -> (String, Unt, List(String)")
CFUNC("getgrnam","getgrnam",  _lib7_P_SysDB_getgrnam,  "String -> (String, Unt,  List(String)")
CFUNC("getpwuid","getpwuid",  _lib7_P_SysDB_getpwuid,  "Unt -> (String, Unt, Unt, String, String)")
CFUNC("getpwnam","getpwnam",  _lib7_P_SysDB_getpwnam,  "String -> (String, Unt, Unt, String, String)")



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

