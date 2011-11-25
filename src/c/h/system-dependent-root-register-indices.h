// system-dependent-root-register-indices.h
//
// The root register indices for various machines.
//
// NROOTS gives the size of the variable-size portion (roots[]) of the
// Task state vector.  Note that the name "roots" is slightly misleading;
// while every entry in the vector must be saved over calls to C, not 
// every entry is a valid root on every entry to C.  The valididity of
// most entries is indicated using the register map convention (via
// ArgRegMap); these entries are valid (and live) iff the corresponding
// bit in the register mask is set (see fps/generic.pkg).  N_ARG_REGS
// gives the number of such entries. The pc, exnfate, current_thread_ptr, and base_pointer
// (if defined) are always valid roots, and the icounter (if defined) never is.
//
// This file gets #included in only one other file:
//
//     src/c/h/runtime-base.h

#ifndef MACHINE_SPECIFIC_ROOT_REGISTER_INDICES_H
#define MACHINE_SPECIFIC_ROOT_REGISTER_INDICES_H

#if defined (TARGET_M68)

#   define NROOTS		8		// d7, a0-a4, d3, pc
#   define N_ARG_REGS		5
#   define PC_INDEX		7
#   define EXN_INDEX		0		// d7
#   define ARG_INDEX		1		// a0
#   define CONT_INDEX		2		// a1
#   define CLOSURE_INDEX	3		// a2
#   define CURRENT_THREAD_INDEX	6           	// d3
#   define LINK_INDEX		4

#elif (defined(TARGET_PWRPC32) || defined(TARGET_RS6000))
#   define NROOTS		24
#   define N_ARG_REGS		19
#   define N_PSEUDO_REGS	2

#   define LINK_INDEX		0	
#   define CLOSURE_INDEX	1	
#   define ARG_INDEX		2	
#   define CONT_INDEX		3	
#   define EXN_INDEX		4	
#   define CURRENT_THREAD_INDEX	5   
#   define BASE_INDEX		6   
#   define PC_INDEX		8

#   define MISC0_INDEX		9	// 24
#   define MISC1_INDEX		10	// 25
#   define MISC2_INDEX		11	// 26
#   define MISC3_INDEX		12	// 27
#   define MISC4_INDEX		13	//  3
#   define MISC5_INDEX		14	//  4
#   define MISC6_INDEX		15	//  5
#   define MISC7_INDEX		16	//  6
#   define MISC8_INDEX		17	//  7
#   define MISC9_INDEX		18	//  8
#   define MISC10_INDEX		19	//  9
#   define MISC11_INDEX		20	// 10
#   define MISC12_INDEX		21	// 11
#   define MISC13_INDEX		22	// 12
#   define MISC14_INDEX		23	// 13


#elif defined(TARGET_SPARC32)

#   define NROOTS		23		// pc, %i0-i5, %g7, %g1-%g3, %l0-%l7, %o0-%o1 %o3-%o4
#   define N_ARG_REGS		19		// exclude base_pointer
#   define N_PSEUDO_REGS	2
#   define PC_INDEX		6
#   define EXN_INDEX		7		// %g7
#   define ARG_INDEX		0		// %i0
#   define CONT_INDEX		1		// %i1
#   define CLOSURE_INDEX	2		// %i2
#   define BASE_INDEX		3		// %i3
#   define CURRENT_THREAD_INDEX	5		// %i5
#   define LINK_INDEX		4		// %g1
#   define MISC0_INDEX		8		// %g2
#   define MISC1_INDEX		9		// %g3
#   define MISC2_INDEX		10		// %o0
#   define MISC3_INDEX		11		// %o1
#   define MISC4_INDEX		12		// %l0
#   define MISC5_INDEX		13		// %l1
#   define MISC6_INDEX		14		// %l2
#   define MISC7_INDEX		15		// %l3
#   define MISC8_INDEX		16		// %l4
#   define MISC9_INDEX		17		// %l5
#   define MISC10_INDEX		18		// %l6
#   define MISC11_INDEX		19		// %l7
#   define MISC12_INDEX		20		// %i4
#   define MISC13_INDEX		21		// %o3
#   define MISC14_INDEX		22		// %o4

#elif defined (TARGET_INTEL32)

#   define NROOTS		26
#   define N_ARG_REGS		23
#   define N_PSEUDO_REGS	2
#   define EXN_INDEX		0		// 8(esp)
#   define ARG_INDEX		1		// ebp
#   define CONT_INDEX		2		// esi
#   define CLOSURE_INDEX	3		// 16(esp)
#   define CURRENT_THREAD_INDEX	4		// 28(esp)
#   define LINK_INDEX		5		// 20(esp)
#   define PC_INDEX		6		// eip
#   define MISC0_INDEX		7		// ebx
#   define MISC1_INDEX		8		// ecx
#   define MISC2_INDEX		9		// edx
// MISCn, where n > 2, is a virtual register
#   define MISC3_INDEX		10		// 40(esp)
#   define MISC4_INDEX		11		// 44(esp)
#   define MISC5_INDEX		12		// 48(esp)
#   define MISC6_INDEX		13		// 52(esp)
#   define MISC7_INDEX		14		// 56(esp)
#   define MISC8_INDEX		15		// 60(esp)
#   define MISC9_INDEX		16		// 64(esp)
#   define MISC10_INDEX		17		// 68(esp)
#   define MISC11_INDEX		18		// 72(esp)
#   define MISC12_INDEX		19		// 76(esp)
#   define MISC13_INDEX		20		// 80(esp)
#   define MISC14_INDEX		21		// 84(esp)
#   define MISC15_INDEX		22		// 88(esp)
#   define MISC16_INDEX		23		// 92(esp)
#   define MISC17_INDEX		24		// 96(esp)
#   define MISC18_INDEX		25		// 100(esp)

#endif

#endif // MACHINE_SPECIFIC_ROOT_REGISTER_INDICES_H



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


