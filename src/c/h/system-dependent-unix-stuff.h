// system-dependent-unix-stuff.h
//
// This file contains operating system dependent paths,
// definitions and feature flags for various UNIX systems.
//
// It should not be included in files that are OS independent.
//
// See the file "machine-dependent/system-dependent-signal-get-set-etc.h" for machine
// and operating system dependencies related to signal handling.
//
// Operating system features:
//
// The following feature symbols may be defined:
//
//   HAS_POSIX_LIBRARIES	if the Mythryl Posix binding is supported.
//   HAS_GETRUSAGE		if OS provides getrusage(2) call
//   HAS_SETITIMER		if OS provides setitimer(2) call
//   HAS_MMAP			if OS provides both mmap(2) and /dev/zero.
//   HAS_ANON_MMAP		if OS provides anonymous mmap(2) (OSF/1)
//   HAS_PARTIAL_MUNMAP		if OS allows unmapping of subranges of a mapped
//				chunk
//   HAS_VM_ALLOCATE		if OS provides vm_allocate (MACH)
//   HAS_SCALBN			if OS provides scalbn(3m) (used by bytecode)
//   HAS_ILOGB			if OS provides ilogb(3m) (used by bytecode)
//   HAS_SELECT			if OS supports BSD style select(2)
//   HAS_POLL			if OS supports SystemV style poll(2)
//   HAS_POSIX_SIGS		if OS provides POSIX sigaction signal interface
//				(including the full sigprocmask interface).
//   HAS_BSD_SIGS		if OS provides BSD sigvec interface (including
//				sigsetmask).
//   HAS_SIGCONTEXT		if signal handlers have a struct sigcontext
//				argument.
//   INCLUDE_SIGINFO_H		include file that contains siginfo (if needed).
//   HAS_UCONTEXT		if signal handlers have a ucontext_t argument.
//   HAS_STRERROR		if the system provides the ISO C strerror function.
//   INT_GIDLIST		if the second argument to getgroups is int[].
//
// Note that only one of the following sets of symbols should be defined:
//   { HAS_MMAP, HAS_ANON_MMAP, HAS_VM_ALLOCATE }
//   { HAS_SELECT, HAS_POLL }
//   { HAS_POSIX_SIGS, HAS_BSD_SIGS }
//   { HAS_SIGCONTEXT, HAS_UCONTEXT }
//
// Some UNIX systems do not support the POSIX libraries (HAS_POSIX_LIBRARIES is
// not defined), in which case, some of the following feature flags may be defined:
//
//   HAS_ACCESS
//   HAS_WAITPID		if OS provides waitpid(2) call (POSIX)
//   HAS_WAIT3			if OS provides the BSD wait3(2) call
//   HAS_SYMLINKS		if OS supports symbolic links; this includes
//				the symlink(2) and readlink(2) calls.
//   HAS_GETCWD			if OS supports getcwd(3) (POSIX)
//   HAS_GETWD			if OS supports getwd(3) (BSD)
//   HAS_CHMOD			if OS supports chmod(2) and fchmod(2)
//   HAS_TRUNCATE		if OS supports truncate(2) and ftruncate(2)
//   HAS_GETHOSTNAME		if OS supports gethostname(2)
//   HAS_GETHOSTID		if OS supports gethostid(2)
//   HAS_SYSINFO		if OS supports SystemV style sysinfo(2)
//   HAS_UNAME_ID		if OS supports uname(2) with machine ID field
//
//   { HAS_GETHOSTID, HAS_SYSINFO, HAS_UNAME_ID }
//   { HAS_WAITPID, HAS_WAIT3 }


#ifndef _LIB7_UNIXDEP_
#define _LIB7_UNIXDEP_

#include "../mythryl-config.h"

////////////////////////////
// Include file paths

// XXX BUGGO FIXME This should be changed to use HAVE_SYS_TIME_H
#define INCLUDE_TIME_H		<sys/time.h>

#if (defined(OPSYS_ULTRIX) || defined(OPSYS_AIX))
#define INCLUDE_FCNTL_H		<fcntl.h>

#elif defined(OPSYS_MACH)
// MACH doesn't have <fcntl.h>; the following defininitions are extracted from
// <sys/file.h>.  This isn't included directly, because it conflicts with
// <unistd.h>.
//
#  define INCLUDE_FCNTL_H               "/dev/null"
#  define FNDELAY       00004           // No delay.
#  define FAPPEND       00010           // Append on each write.
#  define FCREAT        01000           // Create if nonexistent.
#  define FTRUNC        02000           // Truncate to zero length.
#  define FEXCL         04000           // Error if already created.
#  define O_RDONLY      000             // Open for reading.
#  define O_WRONLY      001             // Open for writing.
#  define O_RDWR        002             // Open for read & write.
#  define O_NDELAY      FNDELAY         // Non-blocking open.
#  define O_APPEND      FAPPEND         // Append on each write.
#  define O_CREAT       FCREAT          // Open with file create.
#  define O_TRUNC       FTRUNC          // Open with truncation.
#  define O_EXCL        FEXCL           // Error on create if file exists.

#else
#define INCLUDE_FCNTL_H		<sys/fcntl.h>
#endif

#if defined(OPSYS_OSF1) || defined(OPSYS_DUNIX) || defined(OPSYS_AIX) || defined(OPSYS_LINUX) || defined(OPSYS_MKLINUX) || defined(OPSYS_FREEBSD) || defined(OPSYS_NETBSD) || defined(OPSYS_NETBSD2) || defined(OPSYS_OPENBSD) || defined(OPSYS_CYGWIN)
#  define INCLUDE_DIRENT_H	<dirent.h>
#elif defined(OPSYS_MACH)
#  define INCLUDE_DIRENT_H	<sys/dir.h>
#else
#  define INCLUDE_DIRENT_H	<sys/dirent.h>
#endif


////////////////////////////
// SunOS 4.1
#if defined(OPSYS_SUNOS) 
#  ifndef OS_NAME				// OS_NAME should have been defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define OS_NAME	"sunos"			// Note that OS_NAME needs to match the list in src/lib/std/src/nj/platform-properties.pkg
#  endif
#  define HAS_POSIX_LIBRARIES
#  define HAS_BSD_SIGS
#  define HAS_GETRUSAGE
#  define HAS_SETITIMER
#  define HAS_MMAP
#  define HAS_PARTIAL_MUNMAP
#  define HAS_SCALBN
#  define HAS_ILOGB
#  define HAS_SELECT
#  define HAS_SIGCONTEXT
#  define INT_GIDLIST		// Second argument to getgroups is int[].

////////////////////////////////////
// SunOS is not quite full POSIX:
//
typedef int ssize_t;

/////////////////////////////////////////////
// These declarations are not in <errno.h>
extern int	sys_nerr;
extern char	*sys_errlist[];

#elif defined(OPSYS_SOLARIS)			// SunOS 5.x 
#  ifndef OS_NAME				// OS_NAME should have been defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define OS_NAME	"solaris"		// Note that OS_NAME needs to match the list in src/lib/std/src/nj/platform-properties.pkg
#  endif
#  define HAS_POSIX_LIBRARIES
#  define HAS_POSIX_SIGS
#  define HAS_SETITIMER
#  define HAS_MMAP
#  define HAS_PARTIAL_MUNMAP
#  define HAS_SCALBN
#  define HAS_ILOGB
#  define HAS_POLL
#  define HAS_UCONTEXT
#  define INCLUDE_SIGINFO_H <siginfo.h>
#  define HAS_STRERROR

// These declarations are not in <errno.h>
//
extern int	sys_nerr;
extern char	*sys_errlist[];

#elif defined(OPSYS_IRIX4)  // IRIX 4.0.x	// NB: OS_NAME is now defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define HAS_POSIX_LIBRARIES
#  define HAS_POSIX_SIGS
#  define HAS_GETRUSAGE
#  define HAS_SETITIMER
#  define HAS_MMAP
#  define HAS_SELECT
#  define HAS_SIGCONTEXT
#  define HAS_STRERROR
#  define PROT_EXEC PROT_EXECUTE

/* IRIX 4.0 is not quite full POSIX */
typedef int ssize_t;

#elif defined(OPSYS_IRIX5)  // IRIX 5.x		// NB: OS_NAME is now defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define HAS_POSIX_LIBRARIES
#  define HAS_POSIX_SIGS
#  define HAS_GETRUSAGE
#  define HAS_SETITIMER
#  define HAS_MMAP
#  define HAS_POLL
#  define HAS_UCONTEXT
#  define INCLUDE_SIGINFO_H <siginfo.h>
#  define HAS_STRERROR

#elif (defined(OPSYS_OSF1) || defined(OPSYS_DUNIX)) // OSF/1 aka Digital Unix	// NB: OS_NAME is now defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define HAS_POSIX_LIBRARIES
#  define HAS_POSIX_SIGS
#  define HAS_GETRUSAGE
#  define HAS_SETITIMER
#  define HAS_ANON_MMAP
#  define HAS_POLL
#  define HAS_SIGCONTEXT
#  define HAS_STRERROR
// What about HAS_PARTIAL_MUNMAP? - Ken Cline

#elif defined(OPSYS_AIX)  // AIX 3.2		// NB: OS_NAME is now defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define HAS_POSIX_LIBRARIES
#  define HAS_POSIX_SIGS
#  define HAS_GETRUSAGE
#  define HAS_MMAP
#  define HAS_PARTIAL_MUNMAP
#  define HAS_POLL
#  define HAS_SIGCONTEXT
#  define HAS_STRERROR

// These declarations are not in <errno.h>
extern int	sys_nerr;
extern char	*sys_errlist[];

#elif defined(OPSYS_DARWIN) && defined(OPSYS_MACOS_10_1) // MacOS X 10.1	// NB: OS_NAME is now defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define HAS_POSIX_LIBRARIES
#  define HAS_POSIX_SIGS
#  define HAS_GETRUSAGE
#  define HAS_SETITIMER
#  define HAS_ANON_MMAP
#  define HAS_SIGCONTEXT
#  define HAS_STRERROR
#  define HAS_SELECT
#  define MAP_ANONYMOUS MAP_ANON

#elif defined(OPSYS_DARWIN) && defined(OPSYS_MACOS_10_2) // MacOS X 10.2	// NB: OS_NAME is now defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define HAS_POSIX_LIBRARIES
#  define HAS_POSIX_SIGS
#  define HAS_GETRUSAGE
#  define HAS_SETITIMER
#  define HAS_ANON_MMAP
#  define HAS_UCONTEXT
#  define HAS_STRERROR
#  define HAS_SELECT
#  define MAP_ANONYMOUS MAP_ANON

#elif defined(OPSYS_HPUX9)  // HPUX 9.0						// NB: OS_NAME is now defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define HAS_POSIX_LIBRARIES
#  define HAS_POSIX_SIGS
#  define HAS_SETITIMER
#  define HAS_ANON_MMAP
#  define HAS_SELECT
#  define HAS_SIGCONTEXT
#  define HAS_STRERROR
#  define _INCLUDE_POSIX_SOURCE
#  define _INCLUDE_HPUX_SOURCE
#  define _INCLUDE_XOPEN_SOURCE
#  define _INCLUDE_AES_SOURCE
#  define _AID_T

// These declarations are not in <errno.h>
//
extern int      sys_nerr;
extern char     *sys_errlist[];

#elif defined(OPSYS_HPUX)  // HPUX 10.0			// NB: OS_NAME is now defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define HAS_POSIX_LIBRARIES
#  define HAS_POSIX_SIGS
#  define HAS_SETITIMER
#  define HAS_ANON_MMAP
#  define HAS_POLL
#  define HAS_SIGCONTEXT
#  define HAS_STRERROR
#  define _HPUX_SOURCE
#  define _INCLUDE_POSIX_SOURCE
#  define _INCLUDE_POSIX4_SOURCE
#  define _INCLUDE_XOPEN_SOURCE
#  define _INCLUDE_XOPEN_SOURCE_EXTENDED
#  define _INCLUDE_AES_SOURCE
#  define _AID_T

// These declarations are not in <errno.h>
//
extern int      sys_nerr;
extern char     *sys_errlist[];

#elif (defined(TARGET_INTEL32) && defined(OPSYS_LINUX))			// NB: OS_NAME is now defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define HAS_POSIX_LIBRARIES
#  define HAS_POSIX_SIGS
#  define HAS_GETRUSAGE
#  define HAS_SETITIMER
#  define HAS_MMAP
#  define HAS_ANON_MMAP
#  define HAS_PARTIAL_MUNMAP
#  define HAS_SELECT
#  define HAS_UCONTEXT
#  define HAS_STRERROR
#  undef  __USE_GNU		// Avoid drawing a compiler warning if __USE_GNU was already defined.
#  define __USE_GNU

#include <features.h>

#elif (defined(TARGET_PWRPC32) && defined(OPSYS_LINUX))			// NB: OS_NAME is now defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define HAS_POSIX_LIBRARIES
#  define HAS_POSIX_SIGS
#  define HAS_GETRUSAGE
#  define HAS_SETITIMER
#  define HAS_MMAP
#  define HAS_ANON_MMAP
#  define HAS_PARTIAL_MUNMAP
#  define HAS_SELECT
#  define HAS_STRERROR

#include <features.h>

#elif defined(OPSYS_FREEBSD)						// NB: OS_NAME is now defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define HAS_POSIX_LIBRARIES
#  define HAS_BSD_SIGS
#  define HAS_GETRUSAGE
#  define HAS_SETITIMER
#  define HAS_ANON_MMAP
#  define HAS_PARTIAL_MUNMAP
#  define HAS_SELECT
#  define HAS_SCALBN
#  define HAS_ILOGB
#  define HAS_SIGCONTEXT
#  define HAS_STRERROR

// FreeBSD uses MAP_ANON for MAP_ANONYMOUS
#  define MAP_ANONYMOUS MAP_ANON

#elif defined(OPSYS_NETBSD2)		// version 2.x		// NB: OS_NAME is now defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define HAS_POSIX_LIBRARIES
#  define HAS_BSD_SIGS
#  define HAS_GETRUSAGE
#  define HAS_SETITIMER
#  define HAS_MMAP
#  define HAS_SELECT
#  define HAS_SCALBN
#  define HAS_ILOGB
#  define HAS_SIGCONTEXT
#  define HAS_STRERROR

#elif defined(OPSYS_NETBSD)		// version 3.x		// NB: OS_NAME is now defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define HAS_POSIX_LIBRARIES
#  define HAS_BSD_SIGS
#  define HAS_GETRUSAGE
#  define HAS_SETITIMER
#  define HAS_MMAP
#  define HAS_SELECT
#  define HAS_SCALBN
#  define HAS_ILOGB
#  define HAS_UCONTEXT
#  define HAS_STRERROR

#elif defined(OPSYS_OPENBSD)						// NB: OS_NAME is now defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define HAS_POSIX_LIBRARIES
#  define HAS_POSIX_SIGS							// don't use HAS_BSD_SIGS...OpenBSD has deprecated them in favor of posix sigs
//#  define HAS_UCONTEXT							// doesn't have Linux-style ucontext stuff
#  define HAS_GETRUSAGE
#  define HAS_SETITIMER
#  define HAS_ANON_MMAP
#  define HAS_PARTIAL_MUNMAP
#  define HAS_SELECT
#  define HAS_SCALBN
#  define HAS_ILOGB
#  define HAS_SIGCONTEXT
#  define HAS_STRERROR

// OpenBSD uses MAP_ANON for MAP_ANONYMOUS
#  define MAP_ANONYMOUS MAP_ANON

#elif (defined(TARGET_INTEL32) && defined(OPSYS_CYGWIN))	// NB: OS_NAME is now defined in   src/c/h/architecture-and-os-names-system-dependent.h
#  define HAS_POSIX_LIBRARIES
#  define HAS_POSIX_SIGS
#  define HAS_GETRUSAGE
#  define HAS_SETITIMER
#  define HAS_MMAP
#  define HAS_PARTIAL_MUNMAP
#  define HAS_SELECT
#  define HAS_SIGCONTEXT
#  define HAS_STRERROR

#include <features.h>

#endif


// Extra #defines
//
#if (defined(HOST_SPARC32) && defined(OPSYS_SUNOS) && (! defined(sparc)))
#  define sparc
#elif ((defined(OPSYS_IRIX4) || defined(OPSYS_IRIX5)) && (! defined(LANGUAGE_C)))
#  define LANGUAGE_C
#endif

#if ((defined(OPSYS_IRIX4) || defined(OPSYS_IRIX5)) && (! defined(__EXTENSIONS__)))
#  define __EXTENSIONS__
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>
#include <errno.h>

#endif // _LIB7_UNIXDEP_



// COPYRIGHT (c) 1992 AT&T Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


