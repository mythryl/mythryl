// runtime-values.h
//
// Macros to translate values between
// their C and Mythryl representations
// etc.
//
// We get #included in both C files and
// also the assembly files
//
//     src/c/machine-dependent/prim.intel32.asm
//     src/c/machine-dependent/prim.sparc32.asm
//     src/c/machine-dependent/prim.pwrpc32.asm
//     src/c/machine-dependent/prim.intel32.masm
//
// In the latter cases _ASM_ will be defined, and we
// are careful to #define only stuff that will work
// in assembly code.

#ifndef RUNTIME_VALUES_H
#define RUNTIME_VALUES_H

#include "runtime-base.h"

/* typedef  void*  Val; */	// Defined in   src/c/h/runtime-base.h

#ifdef _ASM_

    #define TAGGED_INT_FROM_C_INT(n)		(((n)*2)+1)

#else

    // When the size of a C pointer differs from
    // the size of a Mythryl value the pointer cast
    // should first convert to a address sized integer
    // before the actual cast.  However this causes
    // problems for gcc when used in a static
    // initialization, hence the PTR_CAST macro.
    //
    #ifdef SIZES_C_64_MYTHRYL_32
	#define PTR_CAST(ty, p)		((ty)(Punt)(p))
    #else
	#define PTR_CAST(ty, p)		((ty)(p))
    #endif

    #define TAGGED_INT_TO_C_INT(n)		(((Val_Sized_Int)(n)) >> 1)
    #define TAGGED_INT_FROM_C_INT(n)		((Val)(((n) << 1) + 1))

    // See also:
    //
    // IS_TAGGED_INT	def in   src/c/h/heap-tags.h

#endif // _ASM_


#ifndef _ASM_

// Convert a Mythryl pointer to
// an Punt value:
//
#define HEAP_POINTER_AS_UNT(p)		((Punt)PTR_CAST(void*, p))

// Fetching tuple fields:
//
#define GET_TUPLE_SLOT_AS_VAL(p, i)	  ((PTR_CAST(Val*, (p)))[i])
#define GET_TUPLE_SLOT_AS_PTR(ty, p, i)	    PTR_CAST(ty,  GET_TUPLE_SLOT_AS_VAL( (p), (i) ))
#define GET_TUPLE_SLOT_AS_INT(p, i)	    TAGGED_INT_TO_C_INT( GET_TUPLE_SLOT_AS_VAL( (p), (i) ))

// Extract the components of a
// ro_vector or rw_vector header:
//
#define GET_VECTOR_DATACHUNK(p)		GET_TUPLE_SLOT_AS_VAL(p, 0)
#define GET_VECTOR_DATACHUNK_AS(ty, p)	GET_TUPLE_SLOT_AS_PTR(ty, p, 0)
#define GET_VECTOR_LENGTH(p)		GET_TUPLE_SLOT_AS_INT(p, 1)

// Turn a Mythryl string into a C string:
//
#define HEAP_STRING_AS_C_STRING(p)		GET_VECTOR_DATACHUNK_AS(char*, p)

// Extract the code address
// from a Mythryl closure:
//
#define GET_CODE_ADDRESS_FROM_CLOSURE(c)	(GET_TUPLE_SLOT_AS_VAL(c, 0))

#endif // _ASM_


// Some basic Mythryl values:
//
#define HEAP_VOID		TAGGED_INT_FROM_C_INT(0)
#define HEAP_FALSE		TAGGED_INT_FROM_C_INT(0)
#define HEAP_TRUE		TAGGED_INT_FROM_C_INT(1)
#define HEAP_NIL		TAGGED_INT_FROM_C_INT(0)

#endif  // RUNTIME_VALUES_H


// COPYRIGHT (c) 1992 AT&T Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


