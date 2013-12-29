// asm-to-c-request-codes.h
//
// The four platform-specific assembly code files
//
//     src/c/machine-dependent/prim.intel32.masm
//     src/c/machine-dependent/prim.sparc32.asm
//     src/c/machine-dependent/prim.intel32.asm
//     src/c/machine-dependent/prim.pwrpc32.asm
//
// implement various functions callable directly
// from Mythryl via the runtime::asm API defined in
//
//     src/lib/core/init/runtime.api
//
// Those assembly code functions may then request services
// from the C level by passing one of the following request
// codes to
//
//     src/c/main/run-mythryl-code-and-runtime-eventloop.c
//
// which then dispatches on them to various fns throughout the C runtime.

//================================
// Nomenclature:
//    We use "make" for functions which both allocate and initialize ram.
//    We use "allot" for functions which allocate uninitialized ram.

#ifndef ASM_TO_C_REQUEST_CODES_H
#define ASM_TO_C_REQUEST_CODES_H

#define REQUEST_HEAPCLEANING	 	 					 0
#define REQUEST_RETURN_TO_C_LEVEL	 					 1
#define REQUEST_HANDLE_UNCAUGHT_EXCEPTION					 2
//
#define REQUEST_FAULT		 	 					 3
#define REQUEST_FIND_CFUN							 4
#define REQUEST_CALL_CFUN		 					 5
//
#define REQUEST_ALLOCATE_STRING	 	 					 6
#define REQUEST_ALLOCATE_BYTE_VECTOR	 					 7
#define REQUEST_ALLOCATE_VECTOR_OF_EIGHT_BYTE_FLOATS	 			 8
//
#define REQUEST_MAKE_TYPEAGNOSTIC_RW_VECTOR					 9
#define REQUEST_MAKE_TYPEAGNOSTIC_RO_VECTOR					10
//
#define REQUEST_RETURN_FROM_SIGNAL_HANDLER					11
#define REQUEST_RESUME_AFTER_RUNNING_SIGNAL_HANDLER				12
//
#define REQUEST_RETURN_FROM_SOFTWARE_GENERATED_PERIODIC_EVENT_HANDLER		13
#define REQUEST_RESUME_AFTER_RUNNING_SOFTWARE_GENERATED_PERIODIC_EVENT_HANDLER	14
#define REQUEST_MAKE_PACKAGE_LITERALS_VIA_BYTECODE_INTERPRETER			15	// This is 'implemented' via die() in src/c/main/run-mythryl-code-and-runtime-eventloop.c
											// -- it is probably obsolete, replaced by  src/c/lib/heap/make-package-literals-via-bytecode-interpreter.c
											// and thus both it and the matching assembly code should be deleted. XXX BUGGO FIXME.
#endif // ASM_TO_C_REQUEST_CODES_H


// COPYRIGHT (c) 1994 AT&T Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.


