/* arena-id.h
 *
 * Definitions for the arena IDs and for mapping from addresses to arena IDs.
 *
 * An arena ID (aid_t) is an unsigned 16-bit value, with the following layout:
 *
 *   bits 0-7:	  heap block ID (0xFF for unmapped chunks)
 *   bits 8-11:   chunk ilk:
 *                  0000 = new-space chunks
 *		    1111 = unmapped chunks
 *   bits 12-15:  generation number (0 for new space, 1-14 for older generations,
 *		  and 15 for non-heap memory)
 *
 * Heap pages in allocation space have the arena ID 0x0000, and unmapped heap
 * pages have the arena ID 0xffff.  The ID format is designed so that a
 * from-space page can be detected by having a generation field less than or
 * equal to the maximum generation being collected.
 */

#ifndef _ARENA_ID_
#define _ARENA_ID_

#ifndef _LIB7_BASE_
#include "runtime-base.h"
#endif

#ifndef _BIBOP_
#include "bibop.h"
#endif

/* The indices of the different chunk arenas.
 * With four bits for the chunk ilk, we have
 * up to 7 regular chunks and up to 7 big-chunk
 * arenas.
 */

/* The different ilk of chunks.
 * Each ilk lives in a different arena:
 */
#define	RECORD_INDEX		0
#define PAIR_INDEX		1
#define STRING_INDEX		2
#define ARRAY_INDEX		3
#define NUM_ARENAS		4

/* The different ilk of big-chunks,
 * which live in big-chunk regions:
 */
#define CODE_INDEX		0
#define NUM_BIGCHUNK_KINDS	1

#define NUM_CHUNK_KINDS		(NUM_ARENAS+NUM_BIGCHUNK_KINDS)

/* Arena IDs: */
typedef page_id_t aid_t;

/* The number of bits in the arena ID fields.
 * The number of bits should add up to sizeof(aid_t)*8.
 */
#define HBLK_BITS		8
#define CHUNKC_BITS		4
#define GEN_BITS		4

#define HBLK_new		0
#define MAX_HBLK		0xff
#define HBLK_MASK		((1<<HBLK_BITS)-1)
#define HBLK_bigchunk		0	/* non-header bigchunk pages */
#define HBLK_bigchunkhdr		1	/* header bigchunk pages */

/* The different ilk of chunks: */
#define MAKE_CHUNKC(INDEX)		(INDEX+1)
#define MAKE_BIGCHUNKC(INDEX)	(0x8|((INDEX)<<1))
#define CHUNKC_new		0x0
#define CHUNKC_record		MAKE_CHUNKC(RECORD_INDEX)
#define CHUNKC_pair		MAKE_CHUNKC(PAIR_INDEX)
#define CHUNKC_string		MAKE_CHUNKC(STRING_INDEX)
#define CHUNKC_array		MAKE_CHUNKC(ARRAY_INDEX)
#define CHUNKC_bigchunk		MAKE_BIGCHUNKC(CODE_INDEX)
#define CHUNKC_MAX		0xf
#define CHUNKC_MASK		((1<<CHUNKC_BITS)-1)

#define CHUNKC_SHIFT		HBLK_BITS
#define GEN_SHIFT		(HBLK_BITS+CHUNKC_BITS)

#define MAX_NUM_GENS		((1 << GEN_BITS) - 2)
#define ALLOC_GEN		0		/* the generation # of the */
						/* allocation arena */

/* Macros on arena IDs: */
#define MAKE_AID(GEN,CHUNKC,BLK)	\
	((aid_t)(((GEN)<<GEN_SHIFT) | ((CHUNKC)<<CHUNKC_SHIFT) | (BLK)))
#define MAKE_MAX_AID(GEN)	MAKE_AID((GEN), CHUNKC_MAX, MAX_HBLK)
#define EXTRACT_HBLK(AID)	((AID)&HBLK_MASK)
#define EXTRACT_CHUNKC(AID)	(((AID) >> CHUNKC_SHIFT)&CHUNKC_MASK)
#define EXTRACT_GEN(AID)	((AID) >> GEN_SHIFT)
#define IS_FROM_SPACE(AID,MAX_AID)	\
	((AID) <= (MAX_AID))

/* The arena IDs for new-space
 * and unmapped heap pages,
 * and for free big-chunks:
 */
#define AID_NEW			MAKE_AID(ALLOC_GEN,CHUNKC_new,HBLK_new)
#define AID_UNMAPPED		PAGEID_unmapped
#define AID_MAX			MAKE_MAX_AID(MAX_NUM_GENS)

#ifdef TOSPACE_ID /* for debugging */
#define TOSPACE_AID(CHUNKC,BLK)	MAKE_AID(0xf,(CHUNKC),BLK)
#define TOSPACE_GEN(AID)	EXTRACT_CHUNKC(AID)
#define IS_TOSPACE_AID(AID)	(((AID) != AID_UNMAPPED) && (EXTRACT_GEN(AID) == 0xf))
#endif

/* AIds for big-chunk regions.
 *
 * These are always marked as from-space,
 * since both from-space and two-space
 * chunks of different generations can
 * occupy the same big-chunk region.
 */
#define AID_BIGCHUNK(GEN)		MAKE_AID(GEN,CHUNKC_bigchunk,HBLK_bigchunk)
#define AID_BIGCHUNK_HDR(GEN)	MAKE_AID(GEN,CHUNKC_bigchunk,HBLK_bigchunkhdr)

/* Return true if the AID is a AID_BIGCHUNK_HDR.
 * (We assume that it is either an AID_BIGCHUNK
 * or an AID_BIGCHUNK_HDR id.)
 */
#define BO_IS_HDR(AID)		(EXTRACT_HBLK(AID) == HBLK_bigchunkhdr)

/* Teturn true, iff AID is a big-chunk AID: */
#define IS_BIGCHUNK_AID(ID)	(EXTRACT_CHUNKC(ID) == CHUNKC_bigchunk)

#endif /* !_ARENA_ID_ */



/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

