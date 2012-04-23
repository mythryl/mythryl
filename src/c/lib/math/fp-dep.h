// fp-dep.h
//
//
// System dependencies for floating-point interface.  This header file defines
// a subset of the X3J11 specification for Floating-point C extensions (March
// 29, 1995 version).  Eventually, we expect that most C compilers will support
// this, and this header file won't be necessary.
//
// The following things are defined:
//
//				Rounding modes:
//	FE_TONEAREST		  to nearest
//	FE_UPWARD		  to +Inf
//	FE_DOWNWARD		  to -Inf
//	FE_TOWARDZERO		  to 0.0
//
//	fe_rnd_mode_t		The representation type of rounding modes.
//
//	fegetround()		Get rounding mode
//	fesetround(rm)		Set rounding mode (return old mode)
//
//	RMODE_EQ(rm1, rm2)	Compare two rounding modes for equality.
//
//	RMODE_C_EQ_LIB7		Set, if the rounding mode values for C are
//				the same as for SML (ignoring representation).

#ifndef _FP_DEP_H_
#define _FP_DEP_H_

#include "system-dependent-stuff.h"

#if defined(HAS_ANSI_C_FP_EXT)

#if HAVE_FENV_H
#  include <fenv.h>
#endif

typedef int fe_rnd_mode_t;

#elif defined(OPSYS_AIX)
#  include <float.h>
#  ifndef FP_RND_RN
  // Some gcc installations screw up the header files, so that <float.h>
  // doesn't contain these definitions.
  //
#    define FP_RND_RZ		0
#    define FP_RND_RN		1
#    define FP_RND_RP		2
#    define FP_RND_RM		3
#  endif
#  define FE_TONEAREST		FP_RND_RN
#  define FE_TOWARDZERO		FP_RND_RZ
#  define FE_UPWARD		FP_RND_RP
#  define FE_DOWNWARD		FP_RND_RM
typedef int fe_rnd_mode_t;
#  define fegetround()		fp_read_rnd()
#  define fesetround(RM)	fp_swap_rnd(RM)

#elif defined(OPSYS_DUNIX)
#  include <float.h>
#  define FE_TONEAREST		FP_RND_RN
#  define FE_TOWARDZERO		FP_RND_RZ
#  define FE_UPWARD		FP_RND_RP
#  define FE_DOWNWARD		FP_RND_RM
typedef int fe_rnd_mode_t;
#  define fegetround()		read_rnd()
#  define fesetround(RM)	write_rnd(RM)

#elif defined(OPSYS_FREEBSD)
#  include <floatingpoint.h>
#  define FE_TONEAREST		FP_RN
#  define FE_TOWARDZERO		FP_RZ
#  define FE_UPWARD		FP_RP
#  define FE_DOWNWARD		FP_RM
typedef int fe_rnd_mode_t;
#  define fegetround()		fpgetround()
#  define fesetround(RM)	fpsetround(RM)

#elif (defined(OPSYS_HPUX) || defined (OPSYS_HPUX9))
#  define _INCLUDE_HPUX_SOURCE
#  include <math.h>
#  define FE_TONEAREST		FP_RN
#  define FE_TOWARDZERO		FP_RZ
#  define FE_UPWARD		FP_RP
#  define FE_DOWNWARD		FP_RM
typedef int fe_rnd_mode_t;
#  define fegetround()		fpgetround()
#  define fesetround(RM)	fpsetround(RM)

#elif defined(OPSYS_HPUX11)
#  define _INCLUDE_HPUX_SOURCE

#if HAVE_FENV_H
#  include <fenv.h>
#endif

typedef int fe_rnd_mode_t;

#elif (defined(OPSYS_IRIX) || defined(OPSYS_IRIX5) || defined(OPSYS_NETBSD) || defined(OPSYS_NETBSD2) || defined(OPSYS_OPENBSD))
#  include <ieeefp.h>
#  define FE_TONEAREST		FP_RN
#  define FE_TOWARDZERO		FP_RZ
#  define FE_UPWARD		FP_RP
#  define FE_DOWNWARD		FP_RM
typedef int fe_rnd_mode_t;
#  define fegetround()		fpgetround()
#  define fesetround(RM)	fpsetround(RM)

#elif (defined(OPSYS_LINUX)  || defined(OPSYS_WIN32) || defined(OPSYS_CYGWIN))
// Linux doesn't provide an API for controlling the rounding modes,
// so we've got to do it by hand.
// Win32 can set (some) alternate math paramters, but then only by re-linking
// with different chunks.  Best to do it by hand here as well.
//
#  define FE_TONEAREST		0
#  define FE_TOWARDZERO		3
#  define FE_UPWARD		2
#  define FE_DOWNWARD		1
typedef int fe_rnd_mode_t;
extern int fegetround (void);
extern int fesetround (int);

#elif defined(OPSYS_MKLINUX)
// We will probably have to write some assembler to support this.
#  define NO_ROUNDING_MODE_CTL

#elif defined(OPSYS_OSF1)
// Because of bugs in Digital's OS (versions earlier than V4.0), rounding
// mode control cannot be supported.  Note that later versions of Digital
// Unix (V4.0+) are called OPSYS_DUNIX.
//
#  define NO_ROUNDING_MODE_CTL

#elif defined(OPSYS_SOLARIS)
#  include <ieeefp.h>
#  define FE_TONEAREST		FP_RN
#  define FE_TOWARDZERO		FP_RZ
#  define FE_UPWARD		FP_RP
#  define FE_DOWNWARD		FP_RM
typedef int fe_rnd_mode_t;
#  if defined(TARGET_INTEL32)   
  // There is a bug in the Solaris Intel32 (x86) implementation of
  // fpgetround() and fpsetround(); we use the assembler code instead.
  //
extern int fegetround (void);
extern int fesetround (int);

#  else
#    define fegetround()          fpgetround()
#    define fesetround(RM)        fpsetround(RM)
#  endif

#elif defined(OPSYS_SUNOS)
#  include <sys/ieeefp.h>
#  include <string.h>
#  define FE_TONEAREST		"nearest"
#  define FE_TOWARDZERO		"tozero"
#  define FE_UPWARD		"positive"
#  define FE_DOWNWARD		"negative"
typedef char *fe_rnd_mode_t;
#  define RMODE_C_NEQ_LIB7

static char *fegetround ()
{
    char	*result;
    ieee_flags("get", "direction", NULL, &result);
    return result;
}

static char *fesetround (char *rm)
{
    char	*result;
    ieee_flags("set", "direction", rm, &result);
    return result;
}
#define RMODE_EQ(RM1, RM2)	(strcmp((RM1), (RM2)) == 0)

#elif defined(OPSYS_DARWIN) && defined(TARGET_PWRPC32)
#   include <architecture/pwrpc32/fp_regs.h>
#   define FE_TONEAREST RN_NEAREST
#   define FE_UPWARD RN_TOWARD_PLUS
#   define FE_DOWNWARD RN_TOWARD_MINUS
#   define FE_TOWARDZERO RN_TOWARD_ZERO
    typedef pwrpc32_fp_rn_t  fe_rnd_mode_t;
    static fe_rnd_mode_t fegetround() { return(get_fp_scr()).rn; }
    static fe_rnd_mode_t fesetround(fe_rnd_mode_t rm) {
	pwrpc32_fp_scr_t fpstate = get_fp_scr();
	fe_rnd_mode_t old = fpstate.rn;
	fpstate.rn = rm;
	set_fp_scr(fpstate);
	return (old);
    }
#else
#  error do not know about FP dependencies
#endif


#ifndef RMODE_EQ
#  define RMODE_EQ(RM1, RM2)	((RM1) == (RM2))
#endif


#ifndef RMODE_C_NEQ_LIB7
#  if ((FE_TONEAREST == 0) && (FE_TOWARDZERO == 1) && (FE_UPWARD == 2) && (FE_DOWNWARD == 3))
#    define RMODE_C_EQ_LIB7
#  endif
#endif

#endif // _FP_DEP_H_





// COPYRIGHT(c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

