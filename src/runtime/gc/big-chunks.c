/* big-chunks.c
 *
 * Code for managing big-chunk regions.
 */

#include "../config.h"

#include "runtime-base.h"
#include "memory.h"
#include "heap.h"
#include "heap-monitor.h"
#include <string.h>

#ifdef BO_DEBUG



void   PrintRegionMap   (bigchunk_region_t* r)
{
    bigchunk_desc_t*	dp;
    bigchunk_desc_t*	dq;
    int			i;

    SayDebug ("[%d] %d/%d, @%#x: ", r->minGen, r->nFree, r->nPages, r->firstPage);

    for (i = 0, dq = NULL;  i < r->nPages;  i++) {

	dp = r->chunkMap[i];

	if (dp != dq) {
	    SayDebug ("|");
	    dq = dp;
	}

	if (BO_IS_FREE(dp))   SayDebug ("_");
	else                  SayDebug ("X");
    }
    SayDebug ("|\n");
}

#endif


bigchunk_desc_t*   BO_AllocRegion   (   heap_t* heap,
                                        Addr_t  szB
                                    )
{
    /* Allocate a big chunk region that is
     * large enough to hold an chunk of at
     * least szB bytes.
     *
     * It returns the descriptor for the
     * free big-chunk that is the region.
     *
     * NOTE: It does not mark the BIBOP entries for the region; this should be
     * done by the caller.
     */

    int		        npages;
    int		        oldNpages;
    int		        i;

    Addr_t	        hdrSzB;
    Addr_t	        heap_chunk_size_in_bytes;

    bigchunk_region_t*  region;
    heap_chunk_t*	heap_chunk;
    bigchunk_desc_t*    desc;

    /* Compute the memory chunk size.
     * NOTE: there probably is a closed form for this,
     * but I'm too lazy to try to figure it out.        XXX BUGGO FIXME
     */
    npages = ROUNDUP(szB, BIGCHUNK_PAGE_SZB) >> BIGCHUNK_PAGE_SHIFT;
    do {
	oldNpages = npages;
	hdrSzB = ROUNDUP(BOREGION_HDR_SZB(npages), BIGCHUNK_PAGE_SZB);
	szB = (npages << BIGCHUNK_PAGE_SHIFT);
	heap_chunk_size_in_bytes = RND_HEAP_CHUNK_SZB(hdrSzB+szB);
	heap_chunk_size_in_bytes = (heap_chunk_size_in_bytes < MIN_BOREGION_SZB) ? MIN_BOREGION_SZB : heap_chunk_size_in_bytes;
	npages = (heap_chunk_size_in_bytes - hdrSzB) >> BIGCHUNK_PAGE_SHIFT;
    } while (npages != oldNpages);

    if (!(heap_chunk = allocate_heap_chunk (heap_chunk_size_in_bytes))) {
	Die ("unable to allocate memory chunk for bigchunk region");
    }
    region = (bigchunk_region_t*) HEAP_CHUNK_BASE( heap_chunk );

    if (!(desc = NEW_CHUNK(bigchunk_desc_t))) {
	Die ("unable to allocate big-chunk descriptor");
    }

    /* Initialize the region header: */
    region->firstPage	= ((Addr_t)region + hdrSzB);
    region->nPages	= npages;
    region->nFree	= npages;
    region->minGen	= MAX_NUM_GENS;
    region->heap_chunk	= heap_chunk;
    region->next	= heap->bigRegions;
    heap->bigRegions	= region;

    heap->numBORegions++;

    for (i = 0;  i < npages;  i++) {
	region->chunkMap[i] = desc;
    }

    /* Initialize the descriptor for the region's memory: */
    desc->chunk		= region->firstPage;
    desc->sizeB		= szB;
    desc->state		= BO_FREE;
    desc->region	= region;

#ifdef BO_DEBUG
SayDebug ("BO_AllocRegion: %d pages @ %#x\n", npages, region->firstPage);
#endif

    return desc;
}



bigchunk_desc_t*   BO_Alloc   (   heap_t*   heap,
                                  int       gen,
                                  Addr_t    chunkSzB
                              )
{
    /* Allocate a big chunk of the given size. */

    bigchunk_desc_t*   header;
    bigchunk_desc_t*   dp;
    bigchunk_desc_t*   newChunk;
    bigchunk_region_t* region;
    Addr_t	       totSzB;
    int		       i;
    int		       npages;
    int		       firstPage;

    totSzB = ROUNDUP(chunkSzB, BIGCHUNK_PAGE_SZB);
    npages = (totSzB >> BIGCHUNK_PAGE_SHIFT);

    /* Search for a free chunk that is big enough (first-fit): */
    header = heap->freeBigChunks;
    for (dp = header->next;  (dp != header) && (dp->sizeB < totSzB);  dp = dp->next)
	continue;

    if (dp == header) {
        /* No free chunk fits, so allocate a new region: */
	dp = BO_AllocRegion (heap, totSzB);
	region = dp->region;

	if (dp->sizeB == totSzB) {

	    /* Allocate the whole region to the chunk: */
	    newChunk = dp;
	} else {

	    /* Split the free chunk: */
	    newChunk		= NEW_CHUNK(bigchunk_desc_t);
	    newChunk->chunk	= dp->chunk;
	    newChunk->region	= region;
	    dp->chunk		= (Addr_t)(dp->chunk) + totSzB;
	    dp->sizeB	       -= totSzB;
	    ADD_BODESC(heap->freeBigChunks, dp);
	    firstPage		= ADDR_TO_BOPAGE(region, newChunk->chunk);

	    for (i = 0;  i < npages;  i++) {
		region->chunkMap[firstPage+i] = newChunk;
            }
	}

    } else if (dp->sizeB == totSzB) {

	REMOVE_BODESC(dp);
	newChunk = dp;
	region = dp->region;

    } else {

        /* Split the free chunk, leaving dp in the free list: */
	region		= dp->region;
	newChunk		= NEW_CHUNK(bigchunk_desc_t);
	newChunk->chunk	= dp->chunk;
	newChunk->region	= region;
	dp->chunk		= (Addr_t)(dp->chunk) + totSzB;
	dp->sizeB	-= totSzB;
	firstPage	= ADDR_TO_BOPAGE(region, newChunk->chunk);

	for (i = 0;  i < npages;  i++) {
	    dp->region->chunkMap[firstPage+i] = newChunk;
        }
    }

    newChunk->sizeB	= chunkSzB;
    newChunk->state	= BO_YOUNG;
    newChunk->gen	= gen;
    region->nFree      -= npages;

    if (region->minGen > gen) {

        /* U0pdate the generation part of the descriptor: */
	region->minGen = gen;

	MarkRegion (BIBOP, (lib7_val_t *)region, HEAP_CHUNK_SZB( region->heap_chunk ),
	    AID_BIGCHUNK(gen)
);

	BIBOP[BIBOP_ADDR_TO_INDEX(region)] = AID_BIGCHUNK_HDR(gen);
    }

#ifdef BO_DEBUG
SayDebug ("BO_Alloc: %d bytes @ %#x\n", chunkSzB, newChunk->chunk);
PrintRegionMap(region);
#endif

    return newChunk;
}


void   BO_Free   (   heap_t* heap,
                     bigchunk_desc_t* desc
                 )
{
    /* Mark a big chunk as free and add it to the free list. */

    bigchunk_region_t *region = desc->region;
    bigchunk_desc_t   *dp;
    int		    firstPage, lastPage, i, j;
    Addr_t	    totSzB = ROUNDUP(desc->sizeB, BIGCHUNK_PAGE_SZB);

    firstPage = ADDR_TO_BOPAGE(region, desc->chunk);
    lastPage = firstPage + (totSzB >> BIGCHUNK_PAGE_SHIFT);

#ifdef BO_DEBUG
SayDebug ("BO_Free: @ %#x, bibop gen = %x, gen = %d, state = %d, pages=[%d..%d)\n",
desc->chunk, (unsigned)EXTRACT_GEN(ADDR_TO_PAGEID(BIBOP, desc->chunk)), desc->gen, desc->state, firstPage, lastPage);
PrintRegionMap(region);
#endif

    if ((firstPage > 0) && BO_IS_FREE(region->chunkMap[firstPage-1])) {

        /* Coalesce with adjacent free chunk: */
	dp = region->chunkMap[firstPage-1];
	REMOVE_BODESC(dp);

	for (i = ADDR_TO_BOPAGE(region, dp->chunk); i < firstPage;  i++) {
	    region->chunkMap[i] = desc;
        }

	desc->chunk = dp->chunk;
	totSzB += dp->sizeB;
	FREE (dp);
    }

    if ((lastPage < region->nPages) && BO_IS_FREE(region->chunkMap[lastPage])) {

        /* Coalesce with adjacent free chunk: */
	dp = region->chunkMap[lastPage];
	REMOVE_BODESC(dp);

	for (i = lastPage, j = i+(dp->sizeB >> BIGCHUNK_PAGE_SHIFT); i < j;  i++) {
	    region->chunkMap[i] = desc;
        }

	totSzB += dp->sizeB;
	FREE (dp);
    }

    desc->sizeB = totSzB;
    desc->state = BO_FREE;

    region->nFree += (lastPage - firstPage);

    /** What if (region->nFree == region->nPages) ??? XXX BUGGO FIXME **/

    /* Add desc to the free list: */
    ADD_BODESC(heap->freeBigChunks, desc);
}


bigchunk_desc_t*   BO_GetDesc   (lib7_val_t addr)
{
    /* Given an address into a big chunk,
     * return the chunk's descriptor.
     */

    bibop_t	    bibop = BIBOP;
    int		    i;
    aid_t	    aid;
    bigchunk_region_t *rp;

    for (i = BIBOP_ADDR_TO_INDEX(addr);  !BO_IS_HDR(aid = bibop[i]);  i--)
	continue;

    rp = (bigchunk_region_t *)BIBOP_INDEX_TO_ADDR(i);

    return ADDR_TO_BODESC(rp, addr);
}



char*   BO_AddrToCodeChunkTag   (Word_t pc)
{
    /* Return the tag of the code chunk containing
     * the given PC, or else NULL.
     */

    bigchunk_region_t	*region;
    aid_t		aid;

    aid = ADDR_TO_PAGEID(BIBOP, pc);

    if (!IS_BIGCHUNK_AID(aid)) return NULL;

    {   int index = BIBOP_ADDR_TO_INDEX( pc );

        while (!BO_IS_HDR(aid)) {
	    aid = BIBOP[--index];
        }
	region = (bigchunk_region_t *)BIBOP_INDEX_TO_ADDR(index);
	return BO_GetCodeChunkTag (ADDR_TO_BODESC(region, pc));
    }
}



Byte_t*   BO_GetCodeChunkTag   (bigchunk_desc_t* bdp)
{
    /* Return the tag of the given code chunk. */

    Byte_t		*lastByte;
    int			kx;

    lastByte = (Byte_t *)(bdp->chunk) + bdp->sizeB - 1;
    kx = *lastByte * WORD_SZB;

    return lastByte - kx + 1;
}

/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
