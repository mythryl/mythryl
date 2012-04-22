// system-dependent-stuff.h
//
// This file contains definitions to hide a few OS dependencies.
// It should be portable across both UNIX and non-UNIX systems,
// unlike the UNIX-specific file
//
//     src/c/h/system-dependent-unix-stuff.h
//
//	GET_HOST_HARDWARE_PAGE_BYTESIZE ()
//	    Return the machine's pagesize in bytes.
//
//	PATH_ARC_SEP
//	    The pathname arc separator character -- / for POSIX, \ for Win32.
//
//	SYSTEM_CALLS_RETURN_ERROR_CODES
//	    This is #define'd on MacOS where system calls typically
//	    return an error code status -- unlike UNIX, where the
//	    global 'errno' is used.
//
//	    This macro is checked only once, in
//		src/c/lib/raise-error.h


#ifndef SYSTEM_DEPENDENT_STUFF_H
#define SYSTEM_DEPENDENT_STUFF_H

#if defined(OPSYS_UNIX)
#  if (defined(OPSYS_SUNOS) || defined(OPSYS_IRIX4) || defined(OPSYS_LINUX) || defined(OPSYS_AIX) || defined(OPSYS_FREEBSD) || defined(OPSYS_NETBSD) || defined(OPSYS_NETBSD2) || defined(OPSYS_OPENBSD) || defined(OPSYS_DARWIN) || defined(OPSYS_CYGWIN))
#     define GET_HOST_HARDWARE_PAGE_BYTESIZE()	(getpagesize())
#  else
   // POSIX 1003.1b interface
#    include "system-dependent-unix-stuff.h"
#    ifdef _SC_PAGESIZE
#      define GET_HOST_HARDWARE_PAGE_BYTESIZE()	(sysconf(_SC_PAGESIZE))
#    else
     // HPUX engineers can't read specs.
#      define GET_HOST_HARDWARE_PAGE_BYTESIZE()	(sysconf(_SC_PAGE_SIZE))
#    endif
#  endif

#  define PATH_ARC_SEP	'/'
#  define HAS_GETTIMEOFDAY

#elif defined(OPSYS_MACOS)
#  define PATH_ARC_SEP	':'
#  define SYSTEM_CALLS_RETURN_ERROR_CODES

#elif defined(OPSYS_OS2)
#  define PATH_ARC_SEP	'\\'

#elif defined(OPSYS_WIN32)
#  define PATH_ARC_SEP	'\\'

extern int GetPageSize (void);								// GetPageSize		def in    src/c/machine-dependent/win32-stuff.c

#  define GET_HOST_HARDWARE_PAGE_BYTESIZE()		GetPageSize()
#  define HAS_GETTIMEOFDAY

#endif

// Support for ANSI C Floating-point extensions:
//
#if defined(OPSYS_DARWIN) && defined(TARGET_INTEL32)
#define HAS_ANSI_C_FP_EXT
#endif

#endif // !SYSTEM_DEPENDENT_STUFF_H



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


