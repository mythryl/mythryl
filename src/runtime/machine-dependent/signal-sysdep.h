/* signal-sysdep.h
 *
 * O.S. and machine dependent signal definitions for UNIX systems:
 *
 *   typedef SigReturn_t        the return type of a signal handler.
 *   typedef SigInfo_t          the signal generation information passed to a
 *                              a signal handler.
 *   typedef SigContext_t       the context info passed to a signal handler.
 *   typedef SigMask_t		the representation of a set of signals
 *
 *   SIG_GetCode(info, scp)	extract the signal generation information
 *   SIG_GetPC(scp)		get the PC from the context
 *   SIG_SetPC(scp, addr)	set the PC in the context to the address
 *   SIG_SetHandler(sig, h)	set the signal handler
 *   SIG_SetDefault(sig)	set the handler for sig to SIG_DFL
 *   SIG_SetIgnore(sig)		set the handler for sig to SIG_IGN
 *   SIG_GetHandler(sig, h)	get the current handler into h
 *   SIG_ClearMask(mask)	clear the given signal mask.
 *   SIG_AddToMask(mask, sig)	Add the given signal to the mask.
 *   SIG_isSet(mask, sig)	Return true, if the signal is in the mask.
 *   SIG_SetMask(mask)		Set the signal mask.
 *   SIG_GetMask(mask)		Get the signal mask into the variable mask.
 *
 *   SIG_FAULT[12]		The signals used to detect faults.
 *
 *   SIG_InitFPE()		This macro is defined to be a routine for
 *				initializing the FPE hardware exception mechanism.
 *
 *   SIG_ResetFPE(scp)		This macro is defined to be a routine for resetting
 *				the signal handling state (or hardware status
 *				registers) on machines that require it; otherwise
 *				it is defined to the empty statement.
 *
 * Predicates on signals, the arguments are (signal, code).
 *   INT_DIVZERO(s, c)
 *   INT_OVFLW(s, c)
 *
 * There are two ways to force a GC when a signal occurs.  For some machines,
 * this is done in an assembly routine called ZeroLimitPtr; for others, this
 * can be done directly by manipulating the signal context.  The following
 * macros are used for this purpose:
 *
 *   USE_ZERO_LIMIT_PTR_FN	If set, then we use the ZeroLimitPtr function.
 *   SIG_SavePC(lib7_state, scp)	Save the PC, so that ZeroLimitPtr can restore it.
 *
 *   SIG_ZeroLimitPtr(scp)	Set the limit pointer in the context to zero.
 *
 * NOTE: Currently SavedPC is a global (so that the asm code in adjust_limit
 * can access it).  Once we have a runtimeLink register that allows dynamic
 * access to the Lib7state, we can move SavedPC to the Lib7 State vector.
 */

#ifndef _SIGNAL_SYSDEP_
#define _SIGNAL_SYSDEP_

#ifndef _LIB7_OSDEP_
#include "runtime-osdep.h"
#endif

#ifndef _LIB7_BASE_
#include "runtime-base.h"	/* for Addr_t */
#endif

#if defined(OPSYS_UNIX)
#  include <signal.h>
#endif

#if defined(HAS_UCONTEXT)
#include <ucontext.h>
#ifdef INCLUDE_SIGINFO_H
#  include INCLUDE_SIGINFO_H
#endif

typedef void SigReturn_t;
typedef siginfo_t *SigInfo_t;
typedef ucontext_t SigContext_t;

#elif defined(HAS_SIGCONTEXT)

typedef int SigInfo_t;
typedef struct sigcontext SigContext_t;
#endif


#if defined(HAS_POSIX_SIGS)
/** POSIX signals **/
#  if defined(HAS_UCONTEXT)
#    define SIG_SetHandler(sig, h)	{       		\
	    struct sigaction __svec;        			\
	    sigfillset(&(__svec.sa_mask));  			\
	    __svec.sa_flags = SA_SIGINFO;			\
	    __svec.sa_sigaction = (h);        			\
	    sigaction ((sig), &__svec, 0);  			\
	}
#    define SIG_SetIgnore(sig)		{			\
	    struct sigaction __svec;        			\
	    __svec.sa_flags = 0;				\
	    __svec.sa_handler = SIG_IGN;        		\
	    sigaction ((sig), &__svec, 0);  			\
	}
#    define SIG_SetDefault(sig)		{			\
	    struct sigaction __svec;        			\
	    __svec.sa_flags = 0;				\
	    __svec.sa_handler = SIG_DFL;        		\
	    sigaction ((sig), &__svec, 0);  			\
	}
#  else
#    define SIG_SetHandler(sig, h)	{       		\
	    struct sigaction __svec;        			\
	    sigfillset(&(__svec.sa_mask));  			\
	    __svec.sa_flags = 0;			\
	    __svec.sa_handler = (h);        			\
	    sigaction ((sig), &__svec, 0);  			\
	}
#    define SIG_SetIgnore(sig)	SIG_SetHandler(sig, SIG_IGN)
#    define SIG_SetDefault(sig)	SIG_SetHandler(sig, SIG_DFL)
#endif
#define SIG_GetHandler(sig, h)  {				\
	struct sigaction __svec;				\
	sigaction ((sig), NULL, &__svec);	\
	(h) = __svec.sa_handler;				\
    }
typedef sigset_t SigMask_t;
#define SIG_ClearMask(mask) 	sigemptyset(&(mask))
#define SIG_AddToMask(mask, s)	sigaddset(&(mask), (s))
#define SIG_isSet(mask, s)	sigismember(&(mask), (s))
#define SIG_SetMask(mask)	sigprocmask(SIG_SETMASK, &(mask), NULL)
#define SIG_GetMask(mask)	sigprocmask(SIG_SETMASK, NULL, &(mask))

#elif defined(HAS_BSD_SIGS)
/** BSD signals **/
#define SIG_SetHandler(sig, h)	{       		\
	struct sigvec __svec;               		\
	__svec.sv_mask = 0xFFFFFFFF;        		\
	__svec.sv_flags = SV_INTERRUPT;			\
	__svec.sv_handler = (h);            		\
	sigvec ((sig), &__svec, 0);         		\
    }
#define SIG_SetIgnore(sig)	SIG_SetHandler(sig, SIG_IGN)
#define SIG_SetDefault(sig)	SIG_SetHandler(sig, SIG_DFL)
#define SIG_GetHandler(sig, h)  {			\
	struct sigvec __svec;				\
	sigvec ((sig), NULL, &__svec);	\
	(h) = __svec.sv_handler;			\
    }
typedef int SigMask_t;
#define SIG_ClearMask(mask)	((mask) = 0)
#define SIG_AddToMask(mask, s)	((mask) |= sigmask(s))
#define SIG_isSet(mask, s)	(((mask) & sigmask(s)) != 0)
#define SIG_SetMask(mask)	sigsetmask(mask)
#define SIG_GetMask(mask)	{			\
	int		__tmpMask;			\
	__tmpMask = 0xFFFFFFFF;				\
	(mask) = sigsetmask(__tmpMask);			\
	sigsetmask(mask);				\
    }
#elif defined(OPSYS_WIN32)
  /* no win32 signals yet */
#else
#  error no way to set signal handler
#endif


/** Machine/OS dependent stuff **/

#if defined(HOST_SPARC)

extern void SetFSR(int);
  /* disable all FP exceptions */
#  define SIG_InitFPE()    SetFSR(0)

#  if defined(OPSYS_SUNOS)
    /** SPARC, SUNOS **/
#    define USE_ZERO_LIMIT_PTR_FN
#    define SIG_FAULT1		SIGFPE
#    define INT_DIVZERO(s, c)	(((s) == SIGFPE) && ((c) == FPE_INTDIV_TRAP))
#    define INT_OVFLW(s, c)	(((s) == SIGFPE) && ((c) == FPE_INTOVF_TRAP))
#    define SIG_GetCode(info, scp)	(info)
#    define SIG_GetPC(scp)	((scp)->sc_pc)
#    define SIG_SetPC(scp, addr)	{			\
	(scp)->sc_pc = (long)(addr);				\
	(scp)->sc_npc = (scp)->sc_pc + 4;			\
    }
#    define SIG_SavePC(lib7_state, scp)	{			\
	SigContext_t	*__scp = (scp);				\
	long		__pc = __scp->sc_pc;			\
	if (__pc+4 != __scp->sc_npc)				\
	  /* the pc is pointing to a delay slot, so back-up	\
	   * to the branch. */					\
	    __pc -= 4;						\
	SavedPC = __pc;						\
    }
     typedef void SigReturn_t;

#  elif defined(OPSYS_SOLARIS)
    /** SPARC, SOLARIS **/
#    define SIG_FAULT1	SIGFPE
#    define INT_DIVZERO(s, c)		(((s) == SIGFPE) && ((c) == FPE_INTDIV))
#    define INT_OVFLW(s, c)		(((s) == SIGFPE) && ((c) == FPE_INTOVF))

#    define SIG_GetCode(info,scp)	((info)->si_code)

#    define SIG_GetPC(scp)		((scp)->uc_mcontext.gregs[REG_PC])
#    define SIG_SetPC(scp, addr)	{			\
	(scp)->uc_mcontext.gregs[REG_PC] = (long)(addr);	\
	(scp)->uc_mcontext.gregs[REG_nPC] = (long)(addr) + 4;	\
    }
#    define SIG_ZeroLimitPtr(scp)	\
	{ (scp)->uc_mcontext.gregs[REG_G4] = 0; }

#  endif

#elif (defined(HOST_RS6000) || defined(HOST_PPC))
#  if defined (OPSYS_AIX)
    /** RS6000 or PPC, AIX **/
#    include <fpxcp.h>
#    define SIG_FAULT1		SIGTRAP

#    define INT_DIVZERO(s, c)	(((s) == SIGTRAP) && ((c) & FP_DIV_BY_ZERO))
#    define INT_OVFLW(s, c)	(((s) == SIGTRAP) && ((c) == 0))
     static int SIG_GetCode (SigInfo_t info, SigContext_t *scp);
#    define SIG_GetPC(scp)	((scp)->sc_jmpbuf.jmp_context.iar)
#    define SIG_SetPC(scp, addr)	\
	{ (scp)->sc_jmpbuf.jmp_context.iar = (long)(addr); }
#    define SIG_ZeroLimitPtr(scp)	\
	{ (scp)->sc_jmpbuf.jmp_context.gpr[15] = 0; }
#    define SIG_ResetFPE(scp)	{						\
	    SigContext_t	*__scp = (scp);					\
	    struct mstsave	*__scj = &(__scp->sc_jmpbuf.jmp_context);	\
	    fp_ctx_t		__flt_ctx;					\
	    __scj->xer &= 0x3fffffff;						\
	    fp_sh_trap_info (__scp, &__flt_ctx);				\
	    fp_sh_set_stat (__scp, (__flt_ctx.fpscr & ~__flt_ctx.trap));	\
	}
     typedef void SigReturn_t;

#  elif defined(OPSYS_DARWIN)
    /* PPC, Darwin */
#    define SIG_InitFPE()        set_fsr()
#    define SIG_ResetFPE(scp)    
#    define SIG_FAULT1           SIGTRAP
#    define INT_DIVZERO(s, c)	 ((s) == SIGTRAP)	/* This needs to be refined */
#    define INT_OVFLW(s, c)	 ((s) == SIGTRAP)	/* This needs to be refined */
   /* info about siginfo_t is missing in the include files 4/17/2001 */
#    define SIG_GetCode(info,scp) 0
#    if defined(OPSYS_MACOS_10_1)
       typedef void SigReturn_t;
#      define SIG_GetPC(scp)	 ((scp)->sc_ir)
#      define SIG_SetPC(scp, addr) {(scp)->sc_ir = (int) addr;}
     /* The offset of 17 is hardwired from reverse engineering the contents of
      * sc_regs. 17 is the offset for register 15.
      */
#      define SIG_ZeroLimitPtr(scp)	\
       {  int * regs = (scp)->sc_regs;	\
	  regs[17] = 0;			\
       }
#    elif defined(OPSYS_MACOS_10_2)
     /* see /usr/include/mach/ppc/thread_status.h */
#      define SIG_GetPC(scp)		((scp)->uc_mcontext->ss.srr0)
#      define SIG_SetPC(scp, addr)	{(scp)->uc_mcontext->ss.srr0 = (int) addr;}
     /* The offset of 17 is hardwired from reverse engineering the contents of
      * sc_regs. 17 is the offset for register 15.
      */
#      define SIG_ZeroLimitPtr(scp)	{  (scp)->uc_mcontext->ss.r15 = 0; }
#    endif
#  elif defined(OPSYS_MKLINUX)
    /* RS6000, MkLinux */

#    include "mklinux-regs.h"
     typedef struct mklinux_ppc_regs SigContext_t;

#    define SIG_FAULT1		SIGILL

#    define INT_DIVZERO(s, c)		(((s) == SIGILL) && ((c) == 0x84000000))
#    define INT_OVFLW(s, c)		(((s) == SIGILL) && ((c) == 0x0))
#    define SIG_GetPC(scp)		((scp)->nip)
#    define SIG_SetPC(scp, addr)	{ (scp)->nip = (long)(addr); }
#    define SIG_ZeroLimitPtr(scp)	{ ((scp)->gpr[15] = 0); }
#    define SIG_GetCode(info,scp)	((scp)->fpscr)
#    define SIG_ResetFPE(scp)		{ (scp)->fpscr = 0x0; }
     typedef void SigReturn_t;

#  elif (defined(TARGET_PPC) && defined(OPSYS_LINUX))
    /* PPC, Linux */

#    include <signal.h>
     typedef struct sigcontext_struct SigContext_t; 

#    define SIG_FAULT1          SIGTRAP

#    define INT_DIVZERO(s, c)           (((s) == SIGTRAP) && (((c) == 0) || ((c) == 0x2000) || ((c) == 0x4000)))
#    define INT_OVFLW(s, c)             (((s) == SIGTRAP) && (((c) == 0) || ((c) == 0x2000) || ((c) == 0x4000)))
#    define SIG_GetPC(scp)              ((scp)->regs->nip)
#    define SIG_SetPC(scp, addr)        { (scp)->regs->nip = (long)(addr); }
#    define SIG_ZeroLimitPtr(scp)       { ((scp)->regs->gpr[15] = 0); } /* limitptr = 15 (see src/runtime/machine-dependent/PPC.prim.asm) */
#    define SIG_GetCode(info,scp)       ((scp)->regs->gpr[PT_FPSCR])
#    define SIG_ResetFPE(scp)           { (scp)->regs->gpr[PT_FPSCR] = 0x0; }
     typedef void SigReturn_t;

#  endif /* HOST_RS6000/HOST_PPC */

#elif defined(HOST_X86)

#  define LIMITPTR_X86OFFSET	3	/* offset (words) of limitptr in Lib7 stack */
					/* frame (see X86.prim.asm) */
extern Addr_t *LIB7_X86Frame;		/* used to get at limitptr */
#  define SIG_InitFPE()    FPEEnable()

#  if (defined(TARGET_X86) && defined(OPSYS_LINUX))
    /** X86, LINUX **/
#    define INTO_OPCODE		0xce	/* the 'into' instruction is a single */
					/* instruction that signals OVERFLOW */

#    define SIG_FAULT1		SIGFPE
#    define SIG_FAULT2		SIGSEGV
#    define INT_DIVZERO(s, c)	((s) == SIGFPE)
#    define INT_OVFLW(s, c)	\
	(((s) == SIGSEGV) && (((Byte_t *)c)[-1] == INTO_OPCODE))

#    define SIG_GetCode(info,scp)	((scp)->uc_mcontext.gregs[REG_EIP])
/* for linux, SIG_GetCode simply returns the address of the fault */
#    define SIG_GetPC(scp)		((scp)->uc_mcontext.gregs[REG_EIP])
#    define SIG_SetPC(scp,addr)		{ (scp)->uc_mcontext.gregs[REG_EIP] = (long)(addr); }
#    define SIG_ZeroLimitPtr(scp)	{ LIB7_X86Frame[LIMITPTR_X86OFFSET] = 0; }

#  elif defined(OPSYS_FREEBSD)
    /** x86, FreeBSD **/
#    define SIG_FAULT1		SIGFPE
#    define INT_DIVZERO(s, c)	(((s) == SIGFPE) && ((c) == FPE_INTDIV_TRAP))
#    define INT_OVFLW(s, c)	(((s) == SIGFPE) && ((c) == FPE_INTOVF_TRAP))

#    define SIG_GetCode(info, scp)	(info)
#    define SIG_GetPC(scp)		((scp)->sc_pc)
#    define SIG_SetPC(scp, addr)	{ (scp)->sc_pc = (long)(addr); }
#    define SIG_ZeroLimitPtr(scp)	{ LIB7_X86Frame[LIMITPTR_X86OFFSET] = 0; }

     typedef void SigReturn_t;

#  elif defined(OPSYS_NETBSD2)
    /** x86, NetBSD (version 2.x) **/
#    define SIG_FAULT1		SIGFPE
#    define SIG_FAULT2		SIGBUS
#    define INT_DIVZERO(s, c)	0
#    define INT_OVFLW(s, c)	(((s) == SIGFPE) || ((s) == SIGBUS))

#    define SIG_GetCode(info, scp)	(info)
#    define SIG_GetPC(scp)		((scp)->sc_pc)
#    define SIG_SetPC(scp, addr)	{ (scp)->sc_pc = (long)(addr); }
#    define SIG_ZeroLimitPtr(scp)	{ LIB7_X86Frame[LIMITPTR_X86OFFSET] = 0; }

     typedef void SigReturn_t;

#  elif defined(OPSYS_NETBSD)
    /** x86, NetBSD (version 3.x) **/
#    define SIG_FAULT1		SIGFPE
#    define SIG_FAULT2		SIGBUS
#    define INT_DIVZERO(s, c)	0
#    define INT_OVFLW(s, c)	(((s) == SIGFPE) || ((s) == SIGBUS))

#    define SIG_GetCode(info, scp)	(info)
#    define SIG_GetPC(scp)		(_UC_MACHINE_PC(scp))
#    define SIG_SetPC(scp, addr)	{ _UC_MACHINE_SET_PC(scp, ((long) (addr))); }
#    define SIG_ZeroLimitPtr(scp)	{ LIB7_X86Frame[LIMITPTR_X86OFFSET] = 0; }

#  elif defined(OPSYS_SOLARIS)
     /** x86, Solaris */

#    define SIG_GetPC(scp)		((scp)->uc_mcontext.gregs[EIP])
#    define SIG_SetPC(scp, addr)	{ (scp)->uc_mcontext.gregs[EIP] = (int)(addr); }
#    define SIG_ZeroLimitPtr(scp)	{ LIB7_X86Frame[LIMITPTR_X86OFFSET] = 0; }

#  elif defined(OPSYS_WIN32)
#    define SIG_ZeroLimitPtr()		{ LIB7_X86Frame[LIMITPTR_X86OFFSET] = 0; }

#  elif defined(OPSYS_CYGWIN)

     typedef void SigReturn_t;
#    define SIG_FAULT1		SIGFPE
#    define SIG_FAULT2		SIGSEGV
#    define INT_DIVZERO(s, c)	((s) == SIGFPE)
#    define SIG_ZeroLimitPtr(scp)  { LIB7_X86Frame[LIMITPTR_X86OFFSET] = 0; }

#  elif defined(OPSYS_DARWIN)
    /** x86, Darwin **/
#    define SIG_FAULT1		SIGFPE
#    define INT_DIVZERO(s, c)	(((s) == SIGFPE) && ((c) == FPE_FLTDIV))
#    define INT_OVFLW(s, c)	(((s) == SIGFPE) && ((c) == FPE_FLTOVF))
    /* see /usr/include/mach/i386/thread_status.h */
#    define SIG_GetCode(info,scp)	((info)->si_code)
#    define SIG_GetPC(scp)		((scp)->uc_mcontext->ss.eip)
#    define SIG_SetPC(scp, addr)	{ (scp)->uc_mcontext->ss.eip = (int) addr; }
#    define SIG_ZeroLimitPtr(scp)	{ LIB7_X86Frame[LIMITPTR_X86OFFSET] = 0; }

#  else
#    error "unknown OPSYS for x86"
#  endif

#endif

#ifndef SIG_InitFPE
#define SIG_InitFPE()		/* nop */
#endif

#ifndef SIG_ResetFPE
#define SIG_ResetFPE(SCP)	/* nop */
#endif

#endif /* !_SIGNAL_SYSDEP_ */


/* COPYRIGHT (c) 2006 The SML/NJ Fellowship.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

