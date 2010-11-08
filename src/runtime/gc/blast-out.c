/* blast-out.c
 */

#include "../config.h"

#include "runtime-osdep.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "runtime-heap-image.h"
#include "c-globals-table.h"
#include "addr-hash.h"
#include "gc.h"
#include "blast-out.h"
#include "heap-output.h"
#include "heap-io.h"


/*
###               "I am one of the culprits who created the problem.
###                I used to write those programs back in the '60s
###                and '70s, and was so proud of the fact that I was
###                able to squeeze a few elements of space by not
###                having to put '19' before the year."
###
###                                         -- Alan Greenspan
*/


#define BLAST_ERROR	LIB7_void

/* local routines */
static lib7_val_t BlastUnboxed (lib7_state_t *lib7_state, lib7_val_t chunk);
static lib7_val_t BlastHeap (lib7_state_t *lib7_state, lib7_val_t chunk, blast_res_t *info);
static lib7_val_t AllocBlastData (lib7_state_t *lib7_state, Addr_t sizeB);


lib7_val_t   BlastOut   (lib7_state_t *lib7_state, lib7_val_t chunk)
{
    /* Linearize an Lib7 chunk into a vector of bytes; return LIB7_void on errors. */

    blast_res_t		res;
    int			gen;
    lib7_val_t		blastedChunk;

    /* Collect allocation space */
    collect_garbage_with_extra_roots (lib7_state, 0, &chunk, NULL);

    gen = GetChunkGen (chunk);

    if (gen == -1) {
      /* unboxed */
	blastedChunk = BlastUnboxed (lib7_state, chunk);
    }
    else { /* a regular Lib7 chunk */
      /* do the blast GC */
/* DEBUG  CheckHeap (lib7_state->lib7_heap, lib7_state->lib7_heap->numGens); */
	res = BlastGC (lib7_state, &chunk, gen);

      /* blast out the image */
	blastedChunk = BlastHeap (lib7_state, chunk, &res);

      /* repair the heap or finish the GC */
	BlastGC_FinishUp (lib7_state, &res);

/* DEBUG CheckHeap (lib7_state->lib7_heap, res.maxGen); */
    }

    return blastedChunk;

} /* end of BlastOut */


/* BlastUnboxed:
 *
 * Blast out an unboxed value.
 */
static lib7_val_t BlastUnboxed (lib7_state_t *lib7_state, lib7_val_t chunk)
{
    lib7_blast_hdr_t  blastHdr;
    int		    szB = sizeof(lib7_image_hdr_t) + sizeof(lib7_blast_hdr_t);
    lib7_val_t	    blastedChunk;
    writer_t	    *wr;

  /* allocate space for the chunk */
    blastedChunk = AllocBlastData (lib7_state, szB);
    wr = WR_OpenMem (PTR_LIB7toC(Byte_t, blastedChunk), szB);

    HeapIO_WriteImageHeader (wr, BLAST_UNBOXED);

    blastHdr.numArenas		= 0;
    blastHdr.numBOKinds		= 0;
    blastHdr.numBORegions	= 0;
    blastHdr.hasCode		= FALSE;
    blastHdr.rootChunk		= chunk;

    WR_Write(wr, &blastHdr, sizeof(blastHdr));

    if (WR_Error(wr))
	return LIB7_void;
    else {
	WR_Free(wr);
	SEQHDR_ALLOC (lib7_state, blastedChunk, DESC_string, blastedChunk, szB);
	return blastedChunk;
    }

} /* end of BlastUnboxed */


/* BlastHeap:
 *
 * Blast out the heap image.
 */
static lib7_val_t BlastHeap (lib7_state_t *lib7_state, lib7_val_t chunk, blast_res_t *info)
{
    heap_t		*heap = lib7_state->lib7_heap;
    int			maxGen = info->maxGen;
    Addr_t		totArenaSzB[NUM_ARENAS], totSzB;
    struct {
	Addr_t		    base;	/* the base address of the arena in the heap */
	Addr_t		    offset;	/* the relative position in the merged */
					/* arena. */
    }			adjust[MAX_NUM_GENS][NUM_ARENAS];
    heap_arena_hdr_t	*p, *arenaHdrs[NUM_CHUNK_KINDS], *arenaHdrsBuf;
    int			arenaHdrSz, i, j, numArenas;
    lib7_val_t	        blastedChunk;
    writer_t		*wr;

  /* compute the arena offsets in the heap image */
    for (i = 0;  i < NUM_ARENAS; i++)
	totArenaSzB[i] = 0;
  /* the embedded literals go first */
    totArenaSzB[STRING_INDEX] = BlastGC_AssignLitAddresses (info, STRING_INDEX, 0);
/* DEBUG SayDebug("%d bytes of string literals\n", totArenaSzB[STRING_INDEX]); */
    for (i = 0;  i < maxGen;  i++) {
	for (j = 0;  j < NUM_ARENAS;  j++) {
	    arena_t	*ap = heap->gen[i]->arena[j];
	    adjust[i][j].offset = totArenaSzB[j];
	    if (isACTIVE(ap)) {
/* DEBUG SayDebug("[%d][%d] base = %#x, nextw = %#x, %d bytes\n", */
/* DEBUG i, j, ap->tospBase, ap->nextw, (Addr_t)(ap->nextw) - (Addr_t)(ap->tospBase)); */
		totArenaSzB[j] += (Addr_t)(ap->nextw) - (Addr_t)(ap->tospBase);
		adjust[i][j].base = (Addr_t)(ap->tospBase);
	    }
	    else
		adjust[i][j].base = 0;
	}
    }
/* DEBUG for (i = 0;  i < NUM_ARENAS;  i++) SayDebug ("arena %d: %d bytes\n", i+1, totArenaSzB[i]); */
    /** WHAT ABOUT THE BIG CHUNKS??? XXX BUGGO FIXME **/

  /* Compute the total size of the blasted chunk */
    for (i = 0, numArenas = 0, totSzB = 0;  i < NUM_ARENAS;  i++) {
	if (totArenaSzB[i] > 0) {
	    numArenas++;
	    totSzB += totArenaSzB[i];
	}
    }
    totSzB += (sizeof(lib7_image_hdr_t) + sizeof(lib7_blast_hdr_t)
		+ (numArenas * sizeof(heap_arena_hdr_t)));
    /** COUNT SPACE FOR BIG CHUNKS **/

  /* include the space for the external symbols */
    totSzB += sizeof(extern_table_hdr_t) + ExportTableSz(info->exportTable);

  /* allocate the heap chunk for the blasted representation, and initialize
   * the writer.
   */
    blastedChunk = AllocBlastData (lib7_state, totSzB);
    wr = WR_OpenMem (PTR_LIB7toC(Byte_t, blastedChunk), totSzB);

  /* initialize the arena headers */
    arenaHdrSz = numArenas * sizeof(heap_arena_hdr_t);
    arenaHdrsBuf = (heap_arena_hdr_t *) MALLOC (arenaHdrSz);
    for (p = arenaHdrsBuf, i = 0;  i < NUM_ARENAS;  i++) {
	if (totArenaSzB[i] > 0) {
	    p->gen		    = 0;
	    p->chunkKind		    = i;
	    p->info.o.baseAddr	    = 0;   /* not used */
	    p->info.o.sizeB	    = totArenaSzB[i];
	    p->info.o.roundedSzB    = -1;  /* not used */
	    p->offset		    = -1;  /* not used */
	    arenaHdrs[i]	    = p;
	    p++;
	}
	else
	    arenaHdrs[i] = NULL;
    }
    /** WHAT ABOUT BIG CHUNKS XXX BUGGO FIXME **/

    /* Blast out the image header: */
    if (HeapIO_WriteImageHeader (wr, BLAST_IMAGE) == FAILURE) {
	FREE (arenaHdrsBuf);
	return BLAST_ERROR;
    }

    /* Blast out the blast header: */
    {
	lib7_blast_hdr_t	header;

	header.numArenas = numArenas;
	header.numBOKinds = 0; /** FIX THIS **/
	header.numBORegions = 0;   /** FIX THIS **/

	if (isEXTERNTAG(chunk)) {
	    ASSERT(numArenas == 0);
	    header.rootChunk = chunk;
	}
	else {
	    aid_t	aid = ADDR_TO_PAGEID(BIBOP, chunk);

	    if (IS_BIGCHUNK_AID(aid)) {
		embchunk_info_t	*p = FindEmbChunk(info->embchunkTable, chunk);

		if ((p == NULL) || (p->kind == USED_CODE)) {
		    Error ("blasting big chunks not implemented\n");
		    FREE (arenaHdrsBuf);
		    return BLAST_ERROR;
		}
		else
		    header.rootChunk = p->relAddr;
	    }
	    else {
		Addr_t	addr = PTR_LIB7toADDR(chunk);
		int	gen = EXTRACT_GEN(aid) - 1;
		int	kind = EXTRACT_CHUNKC(aid) - 1;
		addr -= adjust[gen][kind].base;
		addr += adjust[gen][kind].offset;
		header.rootChunk = HIO_TAG_PTR(kind, addr);
	    }
	}

	WR_Write(wr, &header, sizeof(header));
	if (WR_Error(wr)) {
	    FREE (arenaHdrsBuf);
	    return BLAST_ERROR;
	}
    }

  /* blast out the externals table */
    if (HeapIO_WriteExterns(wr, info->exportTable) == -1) {
	FREE (arenaHdrsBuf);
	return BLAST_ERROR;
    }

  /* blast out the arena headers */
    WR_Write (wr, arenaHdrsBuf, arenaHdrSz);
    if (WR_Error(wr)) {
	FREE (arenaHdrsBuf);
	return BLAST_ERROR;
    }

  /* blast out the heap itself */
    for (i = 0;  i < NUM_ARENAS;  i++) {
	if (i == STRING_INDEX) {
	  /* blast out the embedded literals */
	    BlastGC_BlastLits (wr);
	  /* blast out the rest of the strings */
	    for (j = 0;  j < maxGen;  j++) {
		arena_t	*ap = heap->gen[j]->arena[i];
		if (isACTIVE(ap)) {
		    WR_Write(wr, ap->tospBase,
			(Addr_t)(ap->nextw)-(Addr_t)(ap->tospBase));
		}
	    } /* end for */
	}
	else {
	    for (j = 0;  j < maxGen;  j++) {
		arena_t	*ap = heap->gen[j]->arena[i];
		lib7_val_t	*p, *top;
		if (isACTIVE(ap)) {
		    for (p = ap->tospBase, top = ap->nextw;  p < top;  p++) {
			lib7_val_t	w = *p;
			if (isBOXED(w)) {
			    aid_t		aid = ADDR_TO_PAGEID(BIBOP, w);
			    if (isUNMAPPED(aid)) {
				w = ExportCSymbol(info->exportTable, w);
				ASSERT (w != LIB7_void);
			    }
			    else if (IS_BIGCHUNK_AID(aid)) {
				embchunk_info_t	*chunkInfo
						    = FindEmbChunk(info->embchunkTable, w);

				if ((chunkInfo == NULL)
				|| (chunkInfo->kind == USED_CODE))
				    Die("blast bigchunk unimplemented");
				else
				    w = chunkInfo->relAddr;
			    }
			    else {
			      /* adjust the pointer */
				int		gen = EXTRACT_GEN(aid)-1;
				int		kind = EXTRACT_CHUNKC(aid)-1;
				Addr_t	addr = PTR_LIB7toADDR(w);
				addr -= adjust[gen][kind].base;
				addr += adjust[gen][kind].offset;
				w = HIO_TAG_PTR(kind, addr);
			    }
			}
			WR_Put(wr, (Word_t)w);
		    }
		}
	    } /* end for */
	}
    }

    FREE (arenaHdrsBuf);

    if (WR_Error(wr))
	return BLAST_ERROR;
    else {
	SEQHDR_ALLOC (lib7_state, blastedChunk, DESC_string, blastedChunk, totSzB);
	return blastedChunk;
    }

} /* end of BlastHeap */


/* AllocBlastData:
 *
 * Allocate some heap memory for blasting an chunk.
 */
static lib7_val_t AllocBlastData (lib7_state_t *lib7_state, Addr_t sizeB)
{
    heap_t	    *heap = lib7_state->lib7_heap;
    int		    nWords = BYTES_TO_WORDS(sizeB);
    lib7_val_t	    desc = MAKE_DESC(nWords, DTAG_raw32);
    lib7_val_t	    res;

/** we probably should allocate space in the big-chunk region for these chunks **/
    if (sizeB < heap->allocSzB-(8*ONE_K)) {
	LIB7_AllocWrite (lib7_state, 0, desc);
	res = LIB7_Alloc (lib7_state, nWords);
	return res;
    }
    else {
	Die ("blasting out of %d bytes not supported yet!  Increase allocation arena size.", sizeB);
    }

} /* end of AllocBlastData */



/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
