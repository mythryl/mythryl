// asm-base.h
//
// Common definitions for assembly files in the Mythryl system.


#ifndef _ASM_BASE_
#define _ASM_BASE_

#include "runtime-base.h"

// Bool values for assembly code:
//
#define FALSE	0
#define TRUE	1

#if (!defined(SYMBOLMAPSTACK_GLOBALS_HAVE_LEADING_UNDERSCORE)) && (defined(OPSYS_SUNOS) || (defined(OPSYS_FREEBSD) && !defined(__ELF__)) || defined(OPSYS_NETBSD) || (defined(OPSYS_NETBSD2) && !defined(__ELF__)) || (defined(OPSYS_OPENBSD) && !defined(__ELF__)) || defined(OPSYS_NEXTSTEP) || defined(OPSYS_WIN32) || defined(OPSYS_DARWIN) || defined(OPSYS_CYGWIN))
#  define SYMBOLMAPSTACK_GLOBALS_HAVE_LEADING_UNDERSCORE
#endif

// We should probably consider factoring this out
// into system-dependent-unix-stuff.h -- John H Reppy		XXX BUGGO FIXME
//
#ifdef SYMBOLMAPSTACK_GLOBALS_HAVE_LEADING_UNDERSCORE
#  define CSYM(ID)	CONCAT(_,ID)
#else
#  define CSYM(ID)	ID
#endif

#if defined(HOST_SPARC32)
#  if defined(OPSYS_SUNOS)
#    include <machine/asm_linkage.h>
#    include <machine/trap.h>
#    undef ENTRY
#  elif defined(OPSYS_SOLARIS)
#    define _ASM
#    include <sys/stack.h>
#    include <sys/trap.h>
#  endif
#  define CGLOBAL(ID)	.global	CSYM(ID)
#  define LABEL(ID)	ID:
#  define ALIGN4        .align 4
#  define WORD(W)       .word W
#  if defined(OPSYS_NEXTSTEP)
#    define TEXT          .text
#    define DATA          .data
#  else
#    define TEXT          .seg "text"
#    define DATA          .seg "data"
#  endif
#  define BEGIN_PROC(P)
#  define END_PROC(P)

#elif (defined(HOST_RS6000) || defined(HOST_PWRPC32))
#  if defined(OPSYS_AIX)
#    define CFUNSYM(ID)	CONCAT(.,ID)
#    define USE_TOC
#   if defined(HOST_RS6000)
#    define GLOBAL(ID)	.globl CSYM(ID)
#   endif
#    define CGLOBAL(ID)	.globl CSYM(ID)
#    define TEXT	.csect [PR]
#    define DATA	.csect [RW]
#    define RO_DATA	.csect [RO]
#    define ALIGN4	.align 2
#    define ALIGN8	.align 3
#    define DOUBLE(V)	.double V
#    define LABEL(ID)   ID:

#  elif (defined(OPSYS_LINUX) && defined(TARGET_PWRPC32))
#    define CFUNSYM(ID)	ID
#    define CGLOBAL(ID)	.globl CSYM(ID)
#    define TEXT	.section ".text"
#    define DATA	.section ".data"
#    define RO_DATA	.section ".rodata"
#    define ALIGN4	.align 2
#    define ALIGN8	.align 3
#    define DOUBLE(V)	.double V
#    define LABEL(ID)	ID:

#  elif (defined(OPSYS_DARWIN) && defined(TARGET_PWRPC32))
#    define CFUNSYM(ID) CSYM(ID)
#    define CGLOBAL(ID)  .globl  CSYM(ID)
#    define TEXT        .text
#    define DATA        .data
#    define RO_DATA     .data
#    define ALIGN4      .align 2
#    define ALIGN8	.align 3
#    define DOUBLE(V)	.double V
#    define LABEL(ID)	ID:
#    define __SC__      @
#  endif

#  define CENTRY(ID)		\
    .globl CFUNSYM(ID) __SC__	\
    LABEL(CFUNSYM(ID))

#elif defined(HOST_INTEL32)
#  if defined(OPSYS_WIN32)
#    include "intel32-masm.h"
#    define WORD(W)     WORD32(W)
#  else
#    define CGLOBAL(ID)	  GLOBAL CSYM(ID)
#    define LABEL(ID)	  CONCAT(ID,:)
#    define IMMED(ID)	  CONCAT($,ID)
#    define ALIGN4        .align 2
#    define WORD(W)       .word W
#    define TEXT          .text
#    define DATA          .data
#    define BEGIN_PROC(P) .ent P
#    define END_PROC(P)	  .end P

#  endif

#else

#  error missing asm definitions

#endif

#ifndef __SC__
#  define __SC__ 	;
#endif

#define ENTRY(ID)				\
    CGLOBAL(ID) __SC__				\
    LABEL(CSYM(ID))

#define ALIGNED_ENTRY(name)			\
    CGLOBAL(name) __SC__			\
    ALIGN4 __SC__				\
    LABEL(CSYM(name))

#endif // _ASM_BASE_



// COPYRIGHT (c) 1992 AT&T Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


