/* gc-util.c
 *
 * Garbage collection utility routines.
 *
 */

#include "../config.h"

#include <stdarg.h>
#include <string.h>
#include "runtime-base.h"
#include "runtime-limits.h"
#include "runtime-values.h"
#include "memory.h"
#include "card-map.h"
#include "heap.h"
#include "heap-monitor.h"


/* NewGeneration:
 *
 * Allocate and partition the space for a generation.
 */
status_t NewGeneration (gen_t *gen)
{
    int		i;
    Addr_t	tot_size;
    lib7_val_t*p;
    heap_chunk_t*	heap_chunk;
    arena_t*	ap;

    /* Compute the total size: */
    for (tot_size = 0, i = 0;  i < NUM_ARENAS;  i++) {
	if (isACTIVE(gen->arena[i]))
	    tot_size += gen->arena[i]->tospSizeB;
    }

    if ((gen->cacheChunk != NULL) && (HEAP_CHUNK_SZB(gen->cacheChunk) >= tot_size)) {
	heap_chunk = gen->cacheChunk;
	gen->cacheChunk =  NULL;
    }
    else if ((heap_chunk = allocate_heap_chunk(tot_size)) == NULL) {
	/** Eventually we should try to allocate the generation as separate
	 ** chunks instead of failing.
	 **/
	return FAILURE;
    }

    /* Initialize the chunks: */
    gen->toChunk = heap_chunk;
#ifdef VERBOSE
SayDebug ("NewGeneration[%d]: tot_size = %d, [%#x, %#x)\n",
gen->genNum, tot_size, HEAP_CHUNK_BASE( heap_chunk ), HEAP_CHUNK_BASE( heap_chunk ) + HEAP_CHUNK_SZB( heap_chunk ));
#endif
    for (p = (lib7_val_t *)HEAP_CHUNK_BASE( heap_chunk ), i = 0;  i < NUM_ARENAS;  i++) {
	ap = gen->arena[i];
	if (isACTIVE(ap)) {
	    ap->tospBase	= p;
	    ap->nextw		= p;
	    ap->sweep_nextw	= p;
	    p = (lib7_val_t *)((Addr_t)p + ap->tospSizeB);
	    ap->tospTop	= p;
	    MarkRegion (BIBOP, ap->tospBase, ap->tospSizeB, ap->id);
	    HeapMon_MarkRegion (gen->heap, ap->tospBase, ap->tospSizeB, ap->id);
#ifdef VERBOSE
SayDebug ("  %#x:  [%#x, %#x)\n", ap->id, ap->nextw, p);
#endif
	}
	else {
	    ap->tospBase	= NULL;
	    ap->nextw		= NULL;
	    ap->sweep_nextw	= NULL;
	    ap->tospTop		= NULL;
	}
    }

    ap = gen->arena[PAIR_INDEX];
    if (isACTIVE(ap)) {
      /* The first slot of pair-space cannot be used, so that poly-equal won't fault */
	*(ap->nextw++) = LIB7_void;
	*(ap->nextw++) = LIB7_void;
	ap->tospBase = ap->nextw;
	ap->tospSizeB -= (2*WORD_SZB);
	ap->sweep_nextw = ap->nextw;
    }   

    return SUCCESS;

} /* end of NewGeneration */


/* FreeGeneration:
 */
void FreeGeneration (heap_t *heap, int g)
{
    gen_t	*gen = heap->gen[g];
    int		i;

    if (gen->fromChunk == NULL)
	return;

#ifdef VERBOSE
SayDebug ("FreeGeneration [%d]: [%#x, %#x)\n", g+1, HEAP_CHUNK_BASE(gen->fromChunk),
HEAP_CHUNK_BASE(gen->fromChunk) + HEAP_CHUNK_SZB(gen->fromChunk));
#endif
    if (g < heap->cacheGen) {
	if (gen->cacheChunk != NULL) {
	    if (HEAP_CHUNK_SZB(gen->cacheChunk) > HEAP_CHUNK_SZB(gen->fromChunk))
		free_heap_chunk (gen->fromChunk);
	    else {
		free_heap_chunk (gen->cacheChunk);
		gen->cacheChunk = gen->fromChunk;
	    }
	}
	else
	    gen->cacheChunk = gen->fromChunk;
    }
    else
	free_heap_chunk (gen->fromChunk);

/** NOTE: since the arenas are contiguous, we could do this in one call **/
    gen->fromChunk = NULL;
    for (i = 0;  i < NUM_ARENAS;  i++) {
	arena_t		*ap = gen->arena[i];
	if (ap->frspBase != NULL) {
	    MarkRegion (BIBOP, ap->frspBase, ap->frspSizeB, AID_UNMAPPED);
	    HeapMon_MarkRegion (heap, ap->frspBase, ap->frspSizeB, AID_UNMAPPED);
	    ap->frspBase = NULL;
	    ap->frspSizeB = 0;
	    ap->frspTop = NULL;
	}
    }

} /* end of FreeGeneration */


/* NewDirtyVector:
 * Bind in a new dirty vector for the given generation, reclaiming the old
 * vector.
 */
void NewDirtyVector (gen_t *gen)
{
    arena_t	*ap = gen->arena[ARRAY_INDEX];
    int		vecSz = (ap->tospSizeB / CARD_SZB);
    int		allocSzB = CARD_MAP_SZ(vecSz);

    if (gen->dirty == NULL) {
	gen->dirty = (card_map_t *)MALLOC(allocSzB);
	gen->dirty->mapSzB = allocSzB;
    }
    else if (allocSzB > gen->dirty->mapSzB) {
	FREE(gen->dirty);
	gen->dirty = (card_map_t *)MALLOC(allocSzB);
	gen->dirty->mapSzB = allocSzB;
    }
    if (gen->dirty == NULL) {
	Die ("unable to malloc dirty vector");
    }
    gen->dirty->baseAddr = ap->tospBase;
    gen->dirty->numCards = vecSz;
#ifndef BIT_CARDS
    memset (gen->dirty->map, CARD_CLEAN, allocSzB - (sizeof(card_map_t) - WORD_SZB));
#else
    memset (gen->dirty->map, 0, allocSzB - (sizeof(card_map_t) - WORD_SZB));
#endif

} /* end of NewDirtyVector. */


/* MarkRegion:
 *
 * Mark the BIBOP entries corresponding to the range [baseAddr, baseAddr+szB)
 * with aid.
 */
void MarkRegion (bibop_t bibop, lib7_val_t *baseAddr, Word_t szB, aid_t aid)
{
#ifdef TWO_LEVEL_MAP
#  error two level map not supported
#else
    int		start = BIBOP_ADDR_TO_INDEX(baseAddr);
    int		end = BIBOP_ADDR_TO_INDEX(((Addr_t)baseAddr)+szB);
#ifdef VERBOSE
/*SayDebug("MarkRegion [%#x..%#x) as %#x\n", baseAddr, ((Addr_t)baseAddr)+szB, aid); */
#endif

    while (start < end) {
	bibop[start++] = aid;
    }
#endif

} /* end of MarkRegion */


/* ScanWeakPtrs:
 *
 * Scan the list of weak pointers, nullifying those that refer to dead
 * (i.e., from-space) chunks.
 */
void ScanWeakPtrs (heap_t *heap)
{
    lib7_val_t	*p, *q, *chunk, desc;

/* SayDebug ("ScanWeakPtrs:\n"); */
    for (p = heap->weakList;  p != NULL;  p = q) {
	q = PTR_LIB7toC(lib7_val_t, UNMARK_PTR(p[0]));
	chunk = (lib7_val_t *)(Addr_t)UNMARK_PTR(p[1]);
/* SayDebug ("  %#x --> %#x ", p+1, chunk); */

	switch (EXTRACT_CHUNKC(ADDR_TO_PAGEID(BIBOP, chunk))) {
	  case CHUNKC_new:
	  case CHUNKC_record:
	  case CHUNKC_string:
	  case CHUNKC_array:
	    desc = chunk[-1];
	    if (desc == DESC_forwarded) {
		p[0] = DESC_weak;
		p[1] = PTR_CtoLib7(FOLLOW_FWDCHUNK(chunk));
/* SayDebug ("forwarded to %#x\n", FOLLOW_FWDCHUNK(chunk)); */
	    }
	    else {
		p[0] = DESC_null_weak;
		p[1] = LIB7_void;
/* SayDebug ("nullified\n"); */
	    }
	    break;
	  case CHUNKC_pair:
	    if (isDESC(desc = chunk[0])) {
		p[0] = DESC_weak;
		p[1] = PTR_CtoLib7(FOLLOW_FWDPAIR(desc, chunk));
/* SayDebug ("(pair) forwarded to %#x\n", FOLLOW_FWDPAIR(desc, chunk)); */
	    }
	    else {
		p[0] = DESC_null_weak;
		p[1] = LIB7_void;
/* SayDebug ("(pair) nullified\n"); */
	    }
	    break;
	  case CHUNKC_bigchunk:
	    Die ("weak big chunk");
	    break;
	} /* end of switch */
    }

    heap->weakList = NULL;

} /* end of ScanWeakPtrs */



/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
