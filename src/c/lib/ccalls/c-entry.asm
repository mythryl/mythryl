// c-entry.asm 


#include "asm-base.h"

#if defined(TARGET_INTEL32)
#define CALL_BIAS 	5
#define cresult	%eax

#elif defined(TARGET_SPARC32)

#define CALL_BIAS	8
#define DELAY 	nop

#endif

// Offsets must match the C declaration of Code_Header
//
#define NARGS_OFFSET	-4

// grabPC:
// routine to return the PC at entry to this function
//
// NOTE: this code must relocatable using bcopy.

#ifdef OPSYS_WIN32

ENTRY_M MACRO name
	PUBLIC &name
	&name LABEL FAR
	EVEN
ENDM

	.386
	.MODEL FLAT


	EXTRN	_last_entry:DWORD
	EXTRN	_no_args_entry:FAR
	EXTRN	_some_args_entry:FAR

	TEXT
	EVEN

#else

	.text

	.align 2
	.globl CSYM(grabPC)
	.globl CSYM(grabPCend)

#endif

#if defined(TARGET_INTEL32)
#if defined(OPSYS_LINUX)
CSYM(grabPC):				// Called from   build_entry   in   src/c/lib/ccalls/ccalls-fns.c
/*->*/	call	grabPCaux		// put pc in %eax
	subl	$CALL_BIAS,%eax		// adjust pc to point at "->"
	lea	CSYM(last_entry),%ecx	// save it
	movl	%eax,(%ecx)
	cmpl	$0,NARGS_OFFSET(%eax)
	jne	some_args
	lea	CSYM(no_args_entry),%ecx
	jmp	%ecx
	// Should never get here.
some_args:
	lea	CSYM(some_args_entry),%ecx
	jmp	%ecx
	// Should never get here.

// WARNING: this is intel32-linux assembler specific!
// Above call must be relative.
//
grabPCaux:
	pop	%eax	// Grab return address.
	push	%eax	// Put it back.
	ret
CSYM(grabPCend):
	nop
#elif defined(OPSYS_WIN32)
	PUBLIC CSYM(grabPCend)
	PUBLIC CSYM(grabPC)
CSYM(grabPC) LABEL FAR
/*->*/	call	grabPCaux		/* put pc in %eax */
	sub	eax,CALL_BIAS		/* adjust pc to point at "->" */
	lea	ecx,CSYM(last_entry)	/* save it */
	mov     dword ptr 0 [ecx],eax
	cmp	dword ptr (NARGS_OFFSET) [eax],0
	jne	some_args
	lea	ecx,CSYM(no_args_entry)
	jmp	ecx
	/* should never get here */
some_args:
	lea	ecx,CSYM(some_args_entry)
	jmp	ecx
	/* should never get here */

grabPCaux:
	pop	eax	/* grab return address */
	push	eax	/* put it back */
	ret
CSYM(grabPCend) LABEL FAR

	DATA
	PUBLIC 	CSYM(asm_entry_szb)
CSYM(asm_entry_szb) DWORD CSYM(grabPCend) - CSYM(grabPC)

#else
#error unknown intel32 opsys
#endif
#elif defined(TARGET_SPARC32)
	.align 	4
CSYM(grabPC):
/*->*/	st	%o0,[%sp-4]		  // Get some temps.
	mov	%o7,%g1			  // Save ret addr in %g1
	call	grabPCaux		  // Call leaves pc in %o7
	DELAY
	mov	%o7,%o0			  // Restore ret addr.
	mov	%g1,%o7
	sub	%o0,CALL_BIAS,%o0	  // Unbias saved pc.
	set	CSYM(last_entry),%g1	  // Store it.
	st	%o0,[%g1]    
	ld	[%o0+NARGS_OFFSET],%o0	  // Get # of args.
	tst	%o0
	ld	[%sp-4],%o0		  // Relinquish temps.
	bnz	some_args
	nop
	set	CSYM(no_args_entry),%g1
	jmp	%g1
	nop
	// Should never get here.
some_args:
	set	CSYM(some_args_entry),%g1
	jmp	%g1
	nop
	// Should never get here.

grabPCaux:
	// Return address is in %o7.
	retl
	DELAY
CSYM(grabPCend):
#else
#error unknown target arch
#endif

#ifdef OPSYS_WIN32
	END
#elif !defined(TARGET_SPARC32)
	.end
#endif


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.

