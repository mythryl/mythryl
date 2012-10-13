// system-dependent-signal-get-set-etc.h
//
// Operating system- and machine-dependent signal definitions.
//
//   Our signal handlers have type
//
//       void   signal_handler   (
//           //
//           int                          signal_code,	// SIGALRM or whatever.
//           Signal_Handler_Info_Arg      info,
//           Signal_Handler_Context_Arg*  context
//       );
//
//   where the specific types may vary from system to system.
//
//   typedef Signal_Set		Representation of a set of signals.
//
//   GET_SIGNAL_CODE( info, context )			Extract the signal generation information.
//
//   GET_SIGNAL_PROGRAM_COUNTER( context )		Get the program counter from the context.
//   SET_SIGNAL_PROGRAM_COUNTER( scp, addr )		Set the program counter in the context to the address.
//
//   SET_SIGNAL_HANDLER( signal, handler )		Set the signal handler for given signal.
//   GET_SIGNAL_HANDLER( signal, handler )		Get the current handler into 'handle'.
//
//   SELECT_SIG_DFL_HANDLING_FOR_SIGNAL( signal )	Set the handler for 'signal' to SIG_DFL.
//   SELECT_SIG_IGN_HANDLING_FOR_SIGNAL( signal )	Set the handler for 'signal' to SIG_IGN.
//
//
//   CLEAR_SIGNAL_SET(  signal_set )			Clear the given signal set.
//   ADD_SIGNAL_TO_SET( signal_set, signal )		Add 'signal' to 'signal_set'.
//   SIGNAL_IS_IN_SET(  signal_set, signal )		Return TRUE iff 'signal' is in 'signal_set'.
//
//   SET_PROCESS_SIGNAL_MASK( signal_set )		Set the signal mask.
//   GET_PROCESS_SIGNAL_MASK( signal_set )		Get the signal mask into the variable mask.
//
//   SIG_FAULT1, SIG_FAULT2				The signals used to detect integer-overflow and divide-by-zero faults.
//
//   SET_UP_FLOATING_POINT_EXCEPTION_HANDLING()		This macro is defined to be a routine for
//							initializing the FPE hardware exception mechanism.
//
//   RESET_FLOATING_POINT_EXCEPTION_HANDLING( scp )	This macro is defined to be a routine for resetting
//							the signal handling state (or hardware status
//							registers) on machines that require it; otherwise
//							it is defined to the empty statement.
//
// Predicates on signals:
//   INT_DIVZERO( signal, code )
//   INT_OVFLW(   signal, code )
//
// There are two ways to force a heapcleaning when a signal occurs.
// For some machines, this is done in an assembly routine called Zero_Heap_Allocation_Limit;
// for others, this can be done directly by manipulating the signal context.
// The following macros are used for this purpose:
//
//   USE_ZERO_LIMIT_PTR_FN	If set, then we use the Zero_Heap_Allocation_Limit function.
//   SIG_SavePC(task, scp)	Save the PC, so that Zero_Heap_Allocation_Limit can restore it.
//
//   ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER(scp)	Set the limit pointer in the context to zero.
//
// NOTE: Currently SavedPC is a global (so that the asm code in adjust_limit
// can access it).  Once we have a runtimeLink register that allows dynamic
// access to the Lib7state, we can move SavedPC to the Lib7 State vector.	XXX BUGGO FIXME
//
//     ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER(scp)
//
//         This macro is used only from within c_signal_handler() in one of
//
//             (only currently supported case:)  src/c/machine-dependent/posix-signal.c
//             (maybe again someday:)            src/c/machine-dependent/win32-fault.c
//             (maybe again someday:)		 src/c/machine-dependent/cygwin-fault.c
//
//         The purpose of this macro is to make the heapcleaner ("garbage collector") run 
//         sooner than it otherwise would by making it look as though we are out of memory
//         in agegroup0 (heap_allocation_pointer > heap_allocation_limit).
//
//         We want to do this because we do actual signal handling at heapcleaner execution time.
//
//         We do *that* because at heapcleaner execution time the heap is in a self-consistent
//         state: heapcleaner probes are run only at the start of a function, when we don't need
//         to worry about half-constructed records or such.
//
//         On all  architectures  heap_allocation_pointer  is kept in a register.
//
//         On most architectures  heap_allocation_limit    is kept in a register also,
//         making the (heap_allocation_pointer < heap_allocation_limit) check very fast.
//         On these architectures ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER() sets
//         heap_allocation_limit to zero by updating the saved register value visible in
//         c_signal_handler via the scp argument.
//
//         On the x86 architecture  heap_allocation_limit    is kept in a stack slot
//         (because the x86 is register-starved -- we only have four working registers
//         left after preallocation of ESP, heap_allocation_pointer==EDI, stdfate==ESI
//	   and stdarg==EBP) so c_signal_handler() needs a way of finding that stack slot.


#ifndef SYSTEM_DEPENDENT_SIGNAL_GET_SET_ETC_H
#define SYSTEM_DEPENDENT_SIGNAL_GET_SET_ETC_H

#include "system-dependent-stuff.h"
#include "runtime-base.h"		// For Punt.

#if defined(OPSYS_UNIX)
    #include <signal.h>
#endif

#if defined(HAS_UCONTEXT)
    //
    #include <ucontext.h>
    //
    #ifdef       INCLUDE_SIGINFO_H
        #include INCLUDE_SIGINFO_H
    #endif
    //
    typedef siginfo_t*  Signal_Handler_Info_Arg;
    typedef ucontext_t  Signal_Handler_Context_Arg;
    //
#elif defined(HAS_SIGCONTEXT)
    //
    typedef int                 Signal_Handler_Info_Arg;
    typedef struct sigcontext   Signal_Handler_Context_Arg;
    //
#endif

// The first case here is the one used on x86 Linux:

#if defined(HAS_POSIX_SIGS)
// POSIX signals
#  if defined(HAS_UCONTEXT)
#    define SET_SIGNAL_HANDLER(sig, h)	{       		\
	    struct sigaction __svec;        			\
	    sigfillset(&(__svec.sa_mask));  			\
	    __svec.sa_flags = SA_SIGINFO;			\
	    __svec.sa_sigaction = (h);        			\
	    sigaction ((sig), &__svec, 0);  			\
	}
#    define SELECT_SIG_IGN_HANDLING_FOR_SIGNAL(sig)		{			\
	    struct sigaction __svec;        			\
	    __svec.sa_flags = 0;				\
	    __svec.sa_handler = SIG_IGN;        		\
	    sigaction ((sig), &__svec, 0);  			\
	}
#    define SELECT_SIG_DFL_HANDLING_FOR_SIGNAL(sig)		{			\
	    struct sigaction __svec;        			\
	    __svec.sa_flags = 0;				\
	    __svec.sa_handler = SIG_DFL;        		\
	    sigaction ((sig), &__svec, 0);  			\
	}
#  else
#    define SET_SIGNAL_HANDLER(sig, h)	{       		\
	    struct sigaction __svec;        			\
	    sigfillset(&(__svec.sa_mask));  			\
	    __svec.sa_flags = 0;			\
	    __svec.sa_handler = (h);        			\
	    sigaction ((sig), &__svec, 0);  			\
	}
#    define SELECT_SIG_IGN_HANDLING_FOR_SIGNAL(sig)	SET_SIGNAL_HANDLER(sig, SIG_IGN)
#    define SELECT_SIG_DFL_HANDLING_FOR_SIGNAL(sig)	SET_SIGNAL_HANDLER(sig, SIG_DFL)
#endif
#define GET_SIGNAL_HANDLER(sig, h)  {				\
	struct sigaction __svec;				\
	sigaction ((sig), NULL, &__svec);	\
	(h) = __svec.sa_handler;				\
    }
typedef sigset_t Signal_Set;
#define CLEAR_SIGNAL_SET(mask) 	sigemptyset(&(mask))
#define ADD_SIGNAL_TO_SET(mask, s)	sigaddset(&(mask), (s))
#define SIGNAL_IS_IN_SET(mask, s)	sigismember(&(mask), (s))
#define SET_PROCESS_SIGNAL_MASK(mask)	sigprocmask(SIG_SETMASK, &(mask), NULL)
#define GET_PROCESS_SIGNAL_MASK(mask)	sigprocmask(SIG_SETMASK, NULL, &(mask))

#elif defined(HAS_BSD_SIGS)
// BSD signals
#define SET_SIGNAL_HANDLER(sig, h)	{       	\
	struct sigvec __svec;               		\
	__svec.sv_mask = 0xFFFFFFFF;        		\
	__svec.sv_flags = SV_INTERRUPT;			\
	__svec.sv_handler = (h);            		\
	sigvec ((sig), &__svec, 0);         		\
    }
#define SELECT_SIG_IGN_HANDLING_FOR_SIGNAL(sig)	SET_SIGNAL_HANDLER(sig, SIG_IGN)
#define SELECT_SIG_DFL_HANDLING_FOR_SIGNAL(sig)	SET_SIGNAL_HANDLER(sig, SIG_DFL)
#define GET_SIGNAL_HANDLER(sig, h)  {			\
	struct sigvec __svec;				\
	sigvec ((sig), NULL, &__svec);	\
	(h) = __svec.sv_handler;			\
    }
typedef int Signal_Set;
#define CLEAR_SIGNAL_SET(mask)	((mask) = 0)
#define ADD_SIGNAL_TO_SET(mask, s)	((mask) |= sigmask(s))
#define SIGNAL_IS_IN_SET(mask, s)	(((mask) & sigmask(s)) != 0)
#define SET_PROCESS_SIGNAL_MASK(mask)	sigsetmask(mask)
#define GET_PROCESS_SIGNAL_MASK(mask)	{		\
	int		__tmpMask;			\
	__tmpMask = 0xFFFFFFFF;				\
	(mask) = sigsetmask(__tmpMask);			\
	sigsetmask(mask);				\
    }
#elif defined(OPSYS_WIN32)
// no win32 signals yet
#else
#  error no way to set signal handler
#endif


/////////////////////////////////////////////////////////////////////////////////////////
// Machine/OS dependent stuff
//

#if defined(HOST_SPARC32)

extern void SetFSR(int);
// Disable all FP exceptions
#  define SET_UP_FLOATING_POINT_EXCEPTION_HANDLING()    SetFSR(0)

#  if defined(OPSYS_SUNOS)
     // SPARC32, SUNOS 
#    define USE_ZERO_LIMIT_PTR_FN
#    define SIG_FAULT1		SIGFPE
#    define INT_DIVZERO(s, c)	(((s) == SIGFPE) && ((c) == FPE_INTDIV_TRAP))
#    define INT_OVFLW(s, c)	(((s) == SIGFPE) && ((c) == FPE_INTOVF_TRAP))
#    define GET_SIGNAL_CODE(info, scp)	(info)
#    define GET_SIGNAL_PROGRAM_COUNTER(scp)	((scp)->sc_pc)
#    define SET_SIGNAL_PROGRAM_COUNTER(scp, addr)	{			\
	(scp)->sc_pc = (long)(addr);				\
	(scp)->sc_npc = (scp)->sc_pc + 4;			\
    }
#    define SIG_SavePC(task, scp)	{			\
	Signal_Handler_Context_Arg* __scp = (scp);				\
	long		__pc = __scp->sc_pc;			\
	if (__pc+4 != __scp->sc_npc)				\
	  /* the pc is pointing to a delay slot, so back-up	\
	   * to the branch. */					\
	    __pc -= 4;						\
	SavedPC = __pc;						\
    }

#  elif defined(OPSYS_SOLARIS)
     // SPARC32, SOLARIS
#    define SIG_FAULT1	SIGFPE
#    define INT_DIVZERO(s, c)		(((s) == SIGFPE) && ((c) == FPE_INTDIV))
#    define INT_OVFLW(s, c)		(((s) == SIGFPE) && ((c) == FPE_INTOVF))

#    define GET_SIGNAL_CODE(info,scp)	((info)->si_code)

#    define GET_SIGNAL_PROGRAM_COUNTER(scp)		((scp)->uc_mcontext.gregs[REG_PC])
#    define SET_SIGNAL_PROGRAM_COUNTER(scp, addr)	{			\
	(scp)->uc_mcontext.gregs[REG_PC] = (long)(addr);	\
	(scp)->uc_mcontext.gregs[REG_nPC] = (long)(addr) + 4;	\
    }
#    define ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER(scp)	\
	{ (scp)->uc_mcontext.gregs[REG_G4] = 0; }

#  endif

#elif (defined(HOST_PWRPC32))

#  if defined(OPSYS_DARWIN)
     // PWRPC32, Darwin
#    define SET_UP_FLOATING_POINT_EXCEPTION_HANDLING()        set_fsr()
#    define RESET_FLOATING_POINT_EXCEPTION_HANDLING(scp)    
#    define SIG_FAULT1           SIGTRAP
#    define INT_DIVZERO(s, c)	 ((s) == SIGTRAP)	/* This needs to be refined */
#    define INT_OVFLW(s, c)	 ((s) == SIGTRAP)	/* This needs to be refined */
     // info about siginfo_t is missing in the include files 4/17/2001
#    define GET_SIGNAL_CODE(info,scp) 0
#    if defined(OPSYS_MACOS_10_1)


#      define GET_SIGNAL_PROGRAM_COUNTER(scp)	 ((scp)->sc_ir)
#      define SET_SIGNAL_PROGRAM_COUNTER(scp, addr) {(scp)->sc_ir = (int) addr;}
     // The offset of 17 is hardwired from reverse engineering the contents of
     // sc_regs. 17 is the offset for register 15.
     //
#      define ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER(scp)	\
       {  int* regs = (scp)->sc_regs;	\
	  regs[17] = 0;			\
       }
#    elif defined(OPSYS_MACOS_10_2)
     // see /usr/include/mach/pwrpc32/thread_status.h
#      define GET_SIGNAL_PROGRAM_COUNTER(scp)		((scp)->uc_mcontext->ss.srr0)
#      define SET_SIGNAL_PROGRAM_COUNTER(scp, addr)	{(scp)->uc_mcontext->ss.srr0 = (int) addr;}
     // The offset of 17 is hardwired from reverse engineering the contents of
     // sc_regs. 17 is the offset for register 15.
     //
#      define ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER(scp)	{  (scp)->uc_mcontext->ss.r15 = 0; }
#    endif
#  elif defined(OPSYS_MKLINUX)
    // RS6000, MkLinux

#    include "mklinux-regs.h"

     typedef   struct mklinux_pwrpc32_regs   Signal_Handler_Context_Arg;

#    define SIG_FAULT1		SIGILL

#    define INT_DIVZERO(s, c)		(((s) == SIGILL) && ((c) == 0x84000000))
#    define INT_OVFLW(s, c)		(((s) == SIGILL) && ((c) == 0x0))
#    define GET_SIGNAL_PROGRAM_COUNTER(scp)		((scp)->nip)
#    define SET_SIGNAL_PROGRAM_COUNTER(scp, addr)	{ (scp)->nip = (long)(addr); }
#    define ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER(scp)	{ ((scp)->gpr[15] = 0); }
#    define GET_SIGNAL_CODE(info,scp)	((scp)->fpscr)
#    define RESET_FLOATING_POINT_EXCEPTION_HANDLING(scp)		{ (scp)->fpscr = 0x0; }


#  elif (defined(TARGET_PWRPC32) && defined(OPSYS_LINUX))
    // PWRPC32, Linux

#    include <signal.h>

     typedef   struct sigcontext_struct   Signal_Handler_Context_Arg; 

#    define SIG_FAULT1          SIGTRAP

#    define INT_DIVZERO(s, c)           (((s) == SIGTRAP) && (((c) == 0) || ((c) == 0x2000) || ((c) == 0x4000)))
#    define INT_OVFLW(s, c)             (((s) == SIGTRAP) && (((c) == 0) || ((c) == 0x2000) || ((c) == 0x4000)))
#    define GET_SIGNAL_PROGRAM_COUNTER(scp)              ((scp)->regs->nip)
#    define SET_SIGNAL_PROGRAM_COUNTER(scp, addr)        { (scp)->regs->nip = (long)(addr); }
#    define ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER(scp)       { ((scp)->regs->gpr[15] = 0); } // heap_allocation_limit = 15 -- see src/c/machine-dependent/prim.pwrpc32.asm
#    define GET_SIGNAL_CODE(info,scp)       ((scp)->regs->gpr[PT_FPSCR])
#    define RESET_FLOATING_POINT_EXCEPTION_HANDLING(scp)           { (scp)->regs->gpr[PT_FPSCR] = 0x0; }


#  endif // HOST_RS6000/HOST_PWRPC32

#elif defined(HOST_INTEL32)

extern Punt* LIB7_intel32Frame;					// Defined in src/c/machine-dependent/prim.intel32.asm.  Used to get at heap_allocation_limit in c_signal_handler() in src/c/machine-dependent/posix-signal.c via our ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER macro.
#  define HEAP_ALLOCATION_LIMIT_intel32OFFSET	3		// Offset (words) of heap_allocation_limit in Mythryl stackframe -- see src/c/machine-dependent/prim.intel32.asm
extern void FPEEnable (void);			// From 						   src/c/machine-dependent/prim.intel32.asm
#  define SET_UP_FLOATING_POINT_EXCEPTION_HANDLING()    FPEEnable()

#  if (defined(TARGET_INTEL32) && defined(OPSYS_LINUX))
     /** INTEL32, LINUX **/
#    define INTO_OPCODE		0xce	// The 'into' instruction is a single
					// instruction that signals OVERFLOW

#    define SIG_FAULT1		SIGFPE
#    define SIG_FAULT2		SIGSEGV
#    define INT_DIVZERO(s, c)	((s) == SIGFPE)
#    define INT_OVFLW(s, c)	(((s) == SIGSEGV) && (((Unt8 *)c)[-1] == INTO_OPCODE))

#    define GET_SIGNAL_CODE(info,scp)	((scp)->uc_mcontext.gregs[REG_EIP])	// For linux, GET_SIGNAL_CODE simply returns the address of the fault
#    define GET_SIGNAL_PROGRAM_COUNTER(scp)		((scp)->uc_mcontext.gregs[REG_EIP])
#    define SET_SIGNAL_PROGRAM_COUNTER(scp,addr)		{ (scp)->uc_mcontext.gregs[REG_EIP] = (long)(addr); }
#    define ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER(scp)	{ task->heap_allocation_limit__ptr_for__c_signal_handler[HEAP_ALLOCATION_LIMIT_intel32OFFSET] = 0; }

#  elif defined(OPSYS_FREEBSD)
     // intel32, FreeBSD
#    define SIG_FAULT1		SIGFPE
#    define INT_DIVZERO(s, c)	(((s) == SIGFPE) && ((c) == FPE_INTDIV_TRAP))
#    define INT_OVFLW(s, c)	(((s) == SIGFPE) && ((c) == FPE_INTOVF_TRAP))

#    define GET_SIGNAL_CODE(info, scp)	(info)
#    define GET_SIGNAL_PROGRAM_COUNTER(scp)		((scp)->sc_pc)
#    define SET_SIGNAL_PROGRAM_COUNTER(scp, addr)	{ (scp)->sc_pc = (long)(addr); }
#    define ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER(scp)	{ task->heap_allocation_limit__ptr_for__c_signal_handler[HEAP_ALLOCATION_LIMIT_intel32OFFSET] = 0; }


#  elif defined(OPSYS_NETBSD2)
     // intel32, NetBSD (version 2.x)
#    define SIG_FAULT1		SIGFPE
#    define SIG_FAULT2		SIGBUS
#    define INT_DIVZERO(s, c)	0
#    define INT_OVFLW(s, c)	(((s) == SIGFPE) || ((s) == SIGBUS))

#    define GET_SIGNAL_CODE(info, scp)	(info)
#    define GET_SIGNAL_PROGRAM_COUNTER(scp)		((scp)->sc_pc)
#    define SET_SIGNAL_PROGRAM_COUNTER(scp, addr)	{ (scp)->sc_pc = (long)(addr); }
#    define ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER(scp)	{ task->heap_allocation_limit__ptr_for__c_signal_handler[HEAP_ALLOCATION_LIMIT_intel32OFFSET] = 0; }


#  elif defined(OPSYS_NETBSD)
     // intel32, NetBSD (version 3.x)
#    define SIG_FAULT1		SIGFPE
#    define SIG_FAULT2		SIGBUS
#    define INT_DIVZERO(s, c)	0
#    define INT_OVFLW(s, c)	(((s) == SIGFPE) || ((s) == SIGBUS))

#    define GET_SIGNAL_CODE(info, scp)	(info)
#    define GET_SIGNAL_PROGRAM_COUNTER(scp)		(_UC_MACHINE_PC(scp))
#    define SET_SIGNAL_PROGRAM_COUNTER(scp, addr)	{ _UC_MACHINE_SET_PC(scp, ((long) (addr))); }
#    define ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER(scp)	{ task->heap_allocation_limit__ptr_for__c_signal_handler[HEAP_ALLOCATION_LIMIT_intel32OFFSET] = 0; }

#  elif defined(OPSYS_OPENBSD)
     // intel32, OpenBSD
#    define SIG_FAULT1    SIGFPE
#    define INT_DIVZERO(s, c)  (((s) == SIGFPE) && ((c) == FPE_INTDIV))
#    define INT_OVFLW(s, c)  (((s) == SIGFPE) && ((c) == FPE_INTOVF))

#    define GET_SIGNAL_CODE(info, scp)  (info)
#    define GET_SIGNAL_PROGRAM_COUNTER(scp)    ((scp)->sc_pc)
#    define SET_SIGNAL_PROGRAM_COUNTER(scp, addr)  { (scp)->sc_pc = (long)(addr); }
#    define ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER(scp)  { task->heap_allocation_limit__ptr_for__c_signal_handler[HEAP_ALLOCATION_LIMIT_intel32OFFSET] = 0; }

#  elif defined(OPSYS_SOLARIS)
     // intel32, Solaris

#    define GET_SIGNAL_PROGRAM_COUNTER(scp)		((scp)->uc_mcontext.gregs[EIP])
#    define SET_SIGNAL_PROGRAM_COUNTER(scp, addr)	{ (scp)->uc_mcontext.gregs[EIP] = (int)(addr); }
#    define ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER(scp)	{ task->heap_allocation_limit__ptr_for__c_signal_handler[HEAP_ALLOCATION_LIMIT_intel32OFFSET] = 0; }

#  elif defined(OPSYS_WIN32)
#    define ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER()		{ task->heap_allocation_limit__ptr_for__c_signal_handler[HEAP_ALLOCATION_LIMIT_intel32OFFSET] = 0; }

#  elif defined(OPSYS_CYGWIN)

     typedef   void   Signal_Handler_Return_Type;

#    define SIG_FAULT1		SIGFPE
#    define SIG_FAULT2		SIGSEGV
#    define INT_DIVZERO(s, c)	((s) == SIGFPE)
#    define ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER(scp)  { task->heap_allocation_limit__ptr_for__c_signal_handler[HEAP_ALLOCATION_LIMIT_intel32OFFSET] = 0; }

#  elif defined(OPSYS_DARWIN)
     // intel32, Darwin
#    define SIG_FAULT1		SIGFPE
#    define INT_DIVZERO(s, c)	(((s) == SIGFPE) && ((c) == FPE_FLTDIV))
#    define INT_OVFLW(s, c)	(((s) == SIGFPE) && ((c) == FPE_FLTOVF))
     // See /usr/include/mach/i386/thread_status.h
#    define GET_SIGNAL_CODE(info,scp)	((info)->si_code)
#    define GET_SIGNAL_PROGRAM_COUNTER(scp)		((scp)->uc_mcontext->ss.eip)
#    define SET_SIGNAL_PROGRAM_COUNTER(scp, addr)	{ (scp)->uc_mcontext->ss.eip = (int) addr; }
#    define ZERO_HEAP_ALLOCATION_LIMIT_FROM_C_SIGNAL_HANDLER(scp)	{ task->heap_allocation_limit__ptr_for__c_signal_handler[HEAP_ALLOCATION_LIMIT_intel32OFFSET] = 0; }

#  else
#    error "unknown OPSYS for intel32"
#  endif

#endif

#ifndef SET_UP_FLOATING_POINT_EXCEPTION_HANDLING
#define SET_UP_FLOATING_POINT_EXCEPTION_HANDLING()		// Nop.
#endif

#ifndef RESET_FLOATING_POINT_EXCEPTION_HANDLING
#define RESET_FLOATING_POINT_EXCEPTION_HANDLING(SCP)	// Nop.
#endif

#endif // SYSTEM_DEPENDENT_SIGNAL_GET_SET_ETC_H


// COPYRIGHT (c) 2006 The SML/NJ Fellowship.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


