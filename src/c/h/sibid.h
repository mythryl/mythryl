// sibid.h
//
// Definitions for sib IDs ("sibids") and for mapping from 64K ram "boooks" to sib IDs.
//
// For nomenclature, motivation and overview see:
//
//     src/A.HEAP.OVERVIEW



#ifndef SIBID_H
#define SIBID_H



// The following #defs distinguish the
// contents of the different sib buffers.
//
// These go in a four-bit field, so we can
// have up to 7 regular sibs (and additionally
// up to 7 hugechunkd sibs):
//
#define	RECORD_ILK		0					// Exported.
#define   PAIR_ILK		1					// Exported.
#define STRING_ILK		2					// Exported.
#define VECTOR_ILK		3					// Exported.
#define MAX_PLAIN_ILKS		4					// Exported.

// Codechunks are currently the only hugechunks:
//
#define CODE__HUGE_ILK		0					// Exported.
#define   MAX_HUGE_ILKS		1					// Exported.

#define TOTAL_ILKS		(MAX_PLAIN_ILKS + MAX_HUGE_ILKS)	// Exported.	



// Sibid
//
// A sib ID is an unsigned 16-bit value containing three bitfields
// which identifies the contents of a given ram "book" (64K region).
//
// It has the layout:
//
//     bit 15 ... 12 11 ... 8  7     ...        0 bit
//        ----------------------------------------
//        |  age    |   kind  |        id        |
//        | 4 bits  |  4 bits |      8 bits      |
//        ----------------------------------------
//
// The 'age'  field tells us which agegroup the book belongs to.
// The 'kind' field tells us what kind of data the book holds.
// The 'id'   field is used only to distinguish HUGECHUNK_DATA from HUGECHUNK_RECORD.
//
// SIBIDs appear to be very nearly 8-bit values; 
// it seems possible that they were originally 8-bit
// but just slightly outgrew that.
//
// Two sib IDs are special:
//
//    0x0000    Books in agegroup0.
//    0xFFFF    Books representing empty address space -- no ram allocated.
//
//   bits 0-7:	  'id' (0xFF for unmapped chunks)
//   bits 8-11:   'kind' -- see *_KIND defs below.
//                  0000 = new-space chunks
//		    1111 = unmapped chunks
//   bits 12-15:  'age': (0 for new space, 1-14 for older agegroups,
//		  and 15 for non-heap memory)
//
// Books in agegroup0 have the sib ID 0x0000.
// Unmapped books     have the sib ID 0xFFFF.
//
// The ID format is designed so that a from-space book can be
// detected by having an 'age' field less than or equal to
// the maximum agegroup being collected.
//
#ifndef _ASM_
    typedef  Unt16   Sibid;
#endif


// The number of bits in the sib ID fields.
// The number of bits should add up to sizeof(Sibid)*8.
//
#define   ID_BITS		8					// 
#define KIND_BITS		4					// 
#define  AGE_BITS		4					// 

#define NEW_ID			0					// 
#define ID_MASK			((1 << ID_BITS) -1)			// 

#define HUGECHUNK_DATA		0					// Non-header hugechunk books.
#define HUGECHUNK_RECORD	1					// Header hugechunk books.

// The different sib kinds:
//
#define KIND_FROM_ILK(      ilk )	     (ilk + 1)			// Arg is one of RECORD_ILK, PAIR_ILK...
#define KIND_FROM_HUGE_ILK( ilk )	(0x8|(ilk << 1))		// Arg is CODE__HUGE_ILK. "0x8|" distinguishes from plain ilk.
    //
    #define NEW_KIND		0x0					// 
    //
    #define RECORD_KIND		KIND_FROM_ILK(     RECORD_ILK )		// 
    #define   PAIR_KIND		KIND_FROM_ILK(       PAIR_ILK )		// 
    #define STRING_KIND		KIND_FROM_ILK(     STRING_ILK )		// 
    #define VECTOR_KIND		KIND_FROM_ILK(     VECTOR_ILK )		// 
    //
    #define   CODE_KIND		KIND_FROM_HUGE_ILK( CODE__HUGE_ILK )	// 
    //

#define KIND_SHIFT		ID_BITS					// 
#define  AGE_SHIFT		(ID_BITS + KIND_BITS)


#define MAX_AGEGROUPS		((1 << AGE_BITS) - 2)
#define AGEGROUP0		0					// Age of agegroup0.




// Macros on sib IDs:
//
#define MAKE_SIBID( age, kind, id )    ((Sibid)(((age)<<AGE_SHIFT) | ((kind)<<KIND_SHIFT) | (id)))
    //
    // Used only in this file and in   src/c/heapcleaner/heapcleaner-initialization.c

#define MAX_KIND		0xf												// Max value fitting in our 4-bit 'kind' sibid bitfield.
#define MAX_ID			0xff												// Max value fitting in our 8-bit 'id'   sibid bitfield.
// 
#define MAKE_MAX_SIBID( age )	MAKE_SIBID((age), MAX_KIND, MAX_ID)								// 
// 

#define KIND_MASK			((1<<KIND_BITS)-1)									// 

#define GET_ID_FROM_SIBID(  sib_id)	   (((sib_id)              ) &   ID_MASK)						// 
#define GET_KIND_FROM_SIBID(sib_id)	   (((sib_id) >> KIND_SHIFT) & KIND_MASK)						// 
#define GET_AGE_FROM_SIBID( sib_id)	   (((sib_id) >>  AGE_SHIFT)            )						// 

#define SIBID_IS_IN_FROMSPACE(sib_id,max_sibid)  ((sib_id) <= (max_sibid))

// The sib IDs for new-space and unmapped
// books, and for free hugechunks.
//
// An address space book is marked 'unmapped'
// iff it does not contain ram allocated via
//     obtain_multipage_ram_region_from_os											// obtain_multipage_ram_region_from_os	def in    src/c/ram/get-multipage-ram-region-from-os-stuff.c
// This includes both empty address space 
// (no ram) and address space containing
// vanilla C code and data.
//
#define NEWSPACE_SIBID			MAKE_SIBID( AGEGROUP0, NEW_KIND, NEW_ID )						// 
#define UNMAPPED_BOOK_SIBID		0xffff											// 
#define MAX_SIBID			MAKE_MAX_SIBID( MAX_AGEGROUPS )								// 


// Sib IDs for hugechunk regions.
//
// These are always marked as from-space,
// since both from-space and to-space
// chunks of different agegroups can
// occupy the same hugechunk region.
//
#define HUGECHUNK_DATA_SIBID(   age )	MAKE_SIBID( age, CODE_KIND, HUGECHUNK_DATA   )						// 
#define HUGECHUNK_RECORD_SIBID( age )	MAKE_SIBID( age, CODE_KIND, HUGECHUNK_RECORD )						// 

// Return true if the SIBID is a HUGECHUNK_RECORD_SIBID.
// (We assume that it known to be is either a
// HUGECHUNK_DATA_SIBID or an HUGECHUNK_RECORD_SIBID id.)
//
#define SIBID_ID_IS_BIGCHUNK_RECORD(sibid)	(GET_ID_FROM_SIBID(  sibid) == HUGECHUNK_RECORD)				// This is the only use of GET_ID_FROM_SIBID.
#define SIBID_KIND_IS_CODE(         sibid)	(GET_KIND_FROM_SIBID(sibid) == CODE_KIND)					// 



#define BOOK_IS_UNMAPPED(ID)	((ID) == UNMAPPED_BOOK_SIBID)									// 



#ifdef TWO_LEVEL_MAP

    #error two level book_to_sibid_global mapping not implemented

#else

    #define LOG2_BOOK_BYTESIZE			16									// 
    #define BOOK_BYTESIZE				(1 << LOG2_BOOK_BYTESIZE)						// 

    #define BOOKROUNDED_BYTESIZE( bytesize )	ROUND_UP_TO_POWER_OF_TWO( bytesize, BOOK_BYTESIZE )		// 

    #define LOG2_BOOK2SIBID_TABLE_SIZE_IN_SLOTS		(BITS_PER_WORD - LOG2_BOOK_BYTESIZE)				// 
    #define BOOK2SIBID_TABLE_SIZE_IN_SLOTS		(1 << LOG2_BOOK2SIBID_TABLE_SIZE_IN_SLOTS)				// 


    #define GET_BOOK_CONTAINING_POINTEE(a)		(((Punt)(a))   >> LOG2_BOOK_BYTESIZE)			// 
    #define ADDRESS_OF_BOOK(i)				((Punt)  ((i) << LOG2_BOOK_BYTESIZE))			// 



    #define SIBID_FOR_POINTER( book_to_sibid, a )	((book_to_sibid)[ GET_BOOK_CONTAINING_POINTEE( a ) ])



    extern Sibid* book_to_sibid_global;		// Defined in   src/c/heapcleaner/heapcleaner-initialization.c
	//        ====================

#endif // TWO_LEVEL_MAP



#endif			// SIBID_H




// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


