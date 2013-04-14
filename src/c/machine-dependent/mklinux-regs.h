// mklinux-regs.h
//
// This defines the layout of the machine registers as they are
// passed to a signal handler in MkLinux on the PowerPC.  It was
// reverse engineered from the files:
//
//	mklinux/src/arch/osfmach3_pwrpc32/kernel/signal.c
//	mklinux/src/include/asm-pwrpc32/ptrace.h
//
// in the MkLinux sources (DR2.1 update 4).
//
// A MkLinux signal handler has the prototype
//
//	void handler (int signr, struct mklinux_pwrpc32_regs *rp);
//

#ifndef _MKLINUX_REGS_H_
#define _MKLINUX_REGS_H_

#include <asm/ptrace.h>

struct mklinux_pwrpc32_regs {
    unsigned long	gpr[32];
    unsigned long	nip;		// aka PC
    unsigned long	msr;
    unsigned long	orig_r3;
    unsigned long	ctr;
    unsigned long	lnk;
    unsigned long	xer;
    unsigned long	ccr;
    unsigned long	mq;
    unsigned long	trap;
    unsigned long	dar;
    unsigned long	dsisr;
    unsigned long	result;
    unsigned long	pad1[4];	// pad to 48 words
    double		fpr[32];
    unsigned long	pad2;
    unsigned long	fpscr;
};

#endif // _MKLINUX_REGS_H_



// COPYRIGHT (c) 1997 Bell Labs, Lucent Technologies.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.


