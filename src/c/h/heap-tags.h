// heap-tags.h  -- Heapchunk tags.
//
// For background discussion see:
//
//     src/A.HEAP.OVERVIEW


//  Maintainer Warning!
//  ===================
// 
//  Much implicit knowledge about heapchunk sizing
//  is buried in the max_words function in:
// 
//      src/lib/compiler/back/low/main/nextcode/pick-nextcode-funs-for-heaplimit-checks.pkg
// 
//  Changes which invalidate this knowledge could
//  result in subtle heap-corruption bugs.


#ifndef TAGS_H
#define TAGS_H

#if defined(_ASM_) && defined(OPSYS_WIN32)
    //
    #define HEXLIT(x)          CONCAT3(0,x,h)
    #define OROP OR
    #define ANDOP AND
#else
    #define HEXLIT(y)          CONCAT(0x,y)
    #define OROP |
    #define ANDOP &
#endif


// ============================
// ========== WARNING =========
//
// Some of this information is replicated in
//
//     src/lib/compiler/back/low/main/main/heap-tags.pkg
//
// ============================


// A-tag definitions
// =================
//
#define  POINTER_ATAG	HEXLIT(0)	//   00 - Pointers.
#define  TAGWORD_ATAG	HEXLIT(2)	//   10 - Tagwords.
#define    INT31_ATAG	HEXLIT(1)	//   01, 11 - 31-bit immediate integers.

#define ATAG_MASK	HEXLIT(3)	// Bits 0-1 are the A-tag.

// Clear/set the low bit of a Mythryl pointer
// to make it look like a 31-bit immediate integer.
//
// (This appears to be used only  in support
// of weak pointers.)
//
#define MARK_POINTER(p)	((Val)((Punt)(p) OROP HEXLIT(1)))
#define UNMARK_POINTER(p)	((Val)((Punt)(p) ANDOP ~HEXLIT(1)))



// Tagwords B-tag and lengthfield extraction:
//
#define BTAG_SHIFT_IN_BITS	2
#define BTAG_WIDTH_IN_BITS	5
#define BTAG_MASK		(((1 << BTAG_WIDTH_IN_BITS)-1) << BTAG_SHIFT_IN_BITS)
#define TAGWORD_LENGTH_FIELD_SHIFT	(BTAG_SHIFT_IN_BITS+BTAG_WIDTH_IN_BITS)
    //
    // WARNING: Above must stay in sync with
    //     src/lib/compiler/back/low/main/main/heap-tags.pkg
    // definition
    //     tag_width = 7;


// B-tag definitions
// =================
//
#define PAIRS_AND_RECORDS_BTAG				HEXLIT(  0 )		// Records (including pairs).
#define RO_VECTOR_HEADER_BTAG				HEXLIT(  1 )		// ro_vector header: length is C-tag.
#define RW_VECTOR_HEADER_BTAG				HEXLIT(  2 )		// rw_vector header: length is C-tag.
#define RW_VECTOR_DATA_BTAG				HEXLIT(  3 )		// typeagnostic rw_vector data.
#define FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG		HEXLIT(  4 )		// 32-bit aligned non-pointer data.
#define EIGHT_BYTE_ALIGNED_NONPOINTER_DATA_BTAG		HEXLIT(  5 )		// 64-bit aligned non-pointer data.
#define WEAK_POINTER_OR_SUSPENSION_BTAG			HEXLIT(  6 )		// "Special chunk" (weak pointer or suspension): length is C-tag.
#define EXTERNAL_REFERENCE_IN_EXPORTED_HEAP_IMAGE_BTAG	HEXLIT( 10 )		// External symbol reference (used in exported heap images).
#define FORWARDED_CHUNK_BTAG				HEXLIT( 1F )		// A chunk fowarded to to-space -- used during cleaning.
    //
    // WARNING: The above must track the
    //     src/lib/compiler/back/low/main/main/heap-tags.pkg
    // definitions
    //	   pairs_and_records_btag			= make_tag 0u0;
    //	   ro_vector_header_btag			= make_tag 0u1;
    //	   rw_vector_header_btag			= make_tag 0u2;
    //	   rw_vector_data_btag				= make_tag 0u3;
    //	   four_byte_aligned_nonpointer_data_btag	= make_tag 0u4;
    //	   eight_byte_aligned_nonpointer_data_btag	= make_tag 0u5;
    //	   weak_pointer_or_suspension_btag		= make_tag 0u6;


// The cleaner does not need to distinguish read-only
// vector data records from other records, so:
//
#define RO_VECTOR_DATA_BTAG  PAIRS_AND_RECORDS_BTAG				// typeagnostic ro_vector data.

// To the cleaner, a refcell
// is just a one-element vector:
//
#define REFCELL_BTAG	  RW_VECTOR_DATA_BTAG					// Reference cell.



// Vector C-tag definitions
// ========================
//
// Our ro_vector and rw_vector headers distinguish
// different kinds of vectors via the C-tag (length)
// field, using the values defined here.  Since the
// headers already distinguish read-only from read-write
// vectors, these C-tags are agnostic on that front:
//
// We use these tags in our typeagnostic equality
// and pretty-printing code.
//
#define TYPEAGNOSTIC_VECTOR_CTAG	HEXLIT( 0 )
#define UNT8_VECTOR_CTAG	HEXLIT( 1 )
#define UNT16_VECTOR_CTAG	HEXLIT( 2 )
#define INT31_VECTOR_CTAG	HEXLIT( 3 )
#define INT32_VECTOR_CTAG	HEXLIT( 4 )	// Never used.
#define FLOAT32_VECTOR_CTAG	HEXLIT( 5 )
#define FLOAT64_VECTOR_CTAG	HEXLIT( 6 )
    //
    // WARNING: Thes above must track the
    //     src/lib/compiler/back/low/main/main/heap-tags.pkg
    // definitions
    //    typeagnostic_vector_ctag = 0;
    //    unt8_vector_ctag	  = 1;
    //    unt16_vector_ctag       = 2;
    //    int31_vector_ctag       = 3;
    //    int32_vector_ctag       = 4;
    //    float32_vector_ctag     = 5;
    //    float64_vector_ctag     = 6;
    //


// Tagword construction
// ====================
//
// Build a heapchunk tagword from a B-tag and a length (or C-tag):
//
#ifndef _ASM_
    //
    #define MAKE_BTAG(t)	(((t) << BTAG_SHIFT_IN_BITS) OROP TAGWORD_ATAG)
    #define MAKE_TAGWORD(l,t)	((Val)(((l) << TAGWORD_LENGTH_FIELD_SHIFT) OROP MAKE_BTAG(t)))
#else
    #define MAKE_BTAG(t)	(((t)*4) + TAGWORD_ATAG)
    #define MAKE_TAGWORD(l,t)	(((l)*128) + MAKE_BTAG(t))
#endif



// Tagword definitions
// ===================

// These ones have length fields:
//
#define PAIR_TAGWORD		MAKE_TAGWORD( 2, PAIRS_AND_RECORDS_BTAG				)
#define EXCEPTION_TAGWORD	MAKE_TAGWORD( 3, PAIRS_AND_RECORDS_BTAG				)
#define REFCELL_TAGWORD		MAKE_TAGWORD( 1, REFCELL_BTAG					)
#define FLOAT64_TAGWORD		MAKE_TAGWORD( 2, EIGHT_BYTE_ALIGNED_NONPOINTER_DATA_BTAG	)

// These ones (ab)use the length
// field to hold a C-tag:
//
#define     FLOAT64_RW_VECTOR_TAGWORD	MAKE_TAGWORD(     FLOAT64_VECTOR_CTAG,   RW_VECTOR_HEADER_BTAG )
#define TYPEAGNOSTIC_RW_VECTOR_TAGWORD	MAKE_TAGWORD( TYPEAGNOSTIC_VECTOR_CTAG,   RW_VECTOR_HEADER_BTAG )
#define TYPEAGNOSTIC_RO_VECTOR_TAGWORD	MAKE_TAGWORD( TYPEAGNOSTIC_VECTOR_CTAG,   RO_VECTOR_HEADER_BTAG )
#define        UNT8_RW_VECTOR_TAGWORD	MAKE_TAGWORD(        UNT8_VECTOR_CTAG,   RW_VECTOR_HEADER_BTAG )
#define        UNT8_RO_VECTOR_TAGWORD	MAKE_TAGWORD(        UNT8_VECTOR_CTAG,   RO_VECTOR_HEADER_BTAG )
#define                STRING_TAGWORD	MAKE_TAGWORD(        UNT8_VECTOR_CTAG,   RO_VECTOR_HEADER_BTAG )	// Yes, identical to preceding line.

#define FORWARDED_CHUNK_TAGWORD	        MAKE_TAGWORD(0, FORWARDED_CHUNK_BTAG)					// Used during cleaning (garbage collection).

// There are two kinds of special chunks:
// suspensions and weak pointers.
//
// The length field of these defines
// the state and kind of special chunk.
//
#define UNEVALUATED_LAZY_SUSPENSION_CTAG	0	// Never referenced.
#define   EVALUATED_LAZY_SUSPENSION_CTAG	1	// Never referenced.
#define                WEAK_POINTER_CTAG	2	// Weak pointer.
#define         NULLED_WEAK_POINTER_CTAG	3	// Nulled weak pointer.
    //
    // Warning: The above must track the
    //     src/lib/compiler/back/low/main/main/heap-tags.pkg
    // definitions
    //    unevaluated_lazy_suspension_ctag = 0;
    //    evaluated_lazy_suspension_ctag   = 1;
    //    weak_pointer_ctag	          = 2;
    //    nulled_weak_pointer_ctag    = 3;
    // and the
    //     src/lib/core/init/core.pkg
    // definitions
    //     unevaluated_lazy_suspension_ctag =  0;
    //       evaluated_lazy_suspension_ctag =  1;
    // and the
    //     src/lib/std/src/nj/weak-reference.pkg
    // definition
    //     weak_pointer_ctag = 2;

#define   EVALUATED_LAZY_SUSPENSION_TAGWORD   MAKE_TAGWORD(   EVALUATED_LAZY_SUSPENSION_CTAG,    WEAK_POINTER_OR_SUSPENSION_BTAG)		// Nowhere referenced.
#define UNEVALUATED_LAZY_SUSPENSION_TAGWORD   MAKE_TAGWORD( UNEVALUATED_LAZY_SUSPENSION_CTAG,    WEAK_POINTER_OR_SUSPENSION_BTAG)		// Nowhere referenced.
#define                WEAK_POINTER_TAGWORD   MAKE_TAGWORD(                WEAK_POINTER_CTAG,    WEAK_POINTER_OR_SUSPENSION_BTAG)
#define         NULLED_WEAK_POINTER_TAGWORD   MAKE_TAGWORD(         NULLED_WEAK_POINTER_CTAG,    WEAK_POINTER_OR_SUSPENSION_BTAG)

// Tests on words:
//   IS_POINTER(W)   -- TRUE iff lower two bits are binary 00
//   IS_INT31(W)     -- TRUE iff lower bit is 1.
//   IS_TAGWORD(W)   -- TRUE iff lower two bits are binary 10
//
#define IS_POINTER(W)	(((Val_Sized_Unt)(W) & ATAG_MASK) == POINTER_ATAG)
#define IS_INT31(W)	(((Val_Sized_Unt)(W) &         1) ==   INT31_ATAG)
#define IS_TAGWORD(W)	(((Val_Sized_Unt)(W) & ATAG_MASK) == TAGWORD_ATAG)

// Extract tagword fields:
//
#define GET_LENGTH_FROM_TAGWORD(tagword)	 (((Val_Sized_Unt)(tagword)) >> TAGWORD_LENGTH_FIELD_SHIFT)
#define   GET_BTAG_FROM_TAGWORD(tagword)	((((Val_Sized_Unt)(tagword)) ANDOP BTAG_MASK) >> BTAG_SHIFT_IN_BITS)
    //
    // WARNING: The above two appear to have hardwired clones in
    //
    //     src/lib/compiler/back/low/main/main/translate-nextcode-to-treecode-g.pkg

#endif // TAGS_H


// COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


