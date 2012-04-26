// architecture-and-os-names-system-dependent.h


#ifndef MACHINE_ID_H
#define MACHINE_ID_H

#include "runtime-base.h"

#if defined(TARGET_PWRPC32)
#  define ARCHITECTURE_NAME	"pwrpc32"
#elif defined(TARGET_SPARC32)
#  define ARCHITECTURE_NAME	"sparc32"
#elif defined(TARGET_INTEL32)
#  define ARCHITECTURE_NAME	"intel32"
#else
#  error unknown architecture type
#endif

#if   defined(OPSYS_UNIX)			// Note that OS_NAME needs to match the list in src/lib/std/src/nj/platform-properties.pkg
#  if   (defined(OPSYS_AIX))			// See also src/c/h/system-dependent-unix-stuff.h
#    define OS_NAME	"aix"
#  elif (defined(OPSYS_DARWIN))
#    define OS_NAME	"darwin"
#  elif (defined(OPSYS_DUNIX))
#    define OS_NAME	"dunix"
#  elif (defined(OPSYS_FREEBSD) || defined(OPSYS_NETBSD) || defined(OPSYS_NETBSD2) || defined(OPSYS_OPENBSD))
#    define OS_NAME	"bsd"
#  elif (defined(OPSYS_HPUX9))
#    define OS_NAME	"hpux"
#  elif (defined(OPSYS_HPUX))
#    define OS_NAME	"hpux"
#  elif (defined(OPSYS_IRIX4) || defined(OPSYS_IRIX5))
#    define OS_NAME	"irix"
#  elif (defined(OPSYS_LINUX))
#    define OS_NAME	"linux"
#  elif (defined(OPSYS_OSF1))
#    define OS_NAME	"osf1"
#  elif (defined(OPSYS_SOLARIS))
#    define OS_NAME	"solaris"
#  elif (defined(OPSYS_SUNOS))
#    define OS_NAME	"sunos"
#  elif (defined(OPSYS_CYGWIN))
#    define OS_NAME	"cygwin"
#  else
#    define OS_NAME	"unix"
#  endif
#elif defined(OPSYS_MACOS)
#  define OS_NAME	"darwin"
#elif defined(OPSYS_BEOS)
#  define OS_NAME	"beos"
#elif defined(OPSYS_WIN32)
#  define OS_NAME	"win32"
#else
#  error unknown operating system
#endif

#endif					// MACHINE_ID_H



// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

