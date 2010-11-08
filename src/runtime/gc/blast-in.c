/* blast-in.c
 *
 */

#include "../config.h"

#include <stdio.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-state.h"
#include "heap.h"
#include "runtime-heap-image.h"
#include "c-globals-table.h"
#include "heap-input.h"


/* local routines */
static status_t ReadImage (lib7_state_t *lib7_state, inbuf_t *bp, lib7_val_t *chunkRef);


/* BlastIn:
 *
 * Build a Lib7 heap chunk from a sequence of bytes; the fd is the underlying
 * file descriptor (== -1, if blasting from a string), buf is any pre-read
 * bytes of data, and nbytesP points to the number of bytes in buf.
 */
lib7_val_t BlastIn (lib7_state_t *lib7_state, Byte_t *buf, long len, bool_t *seen_error)
{
    inbuf_t		inBuf;
    lib7_image_hdr_t	header;
    lib7_val_t		chunk;

    inBuf.needsSwap	= FALSE;
    inBuf.file		= NULL;
    inBuf.base		= buf;
    inBuf.buf		= buf;
    inBuf.nbytes	= len;

    /* Read the chunk header: */
    if (HeapIO_ReadBlock (&inBuf, &header, sizeof(header)) == FAILURE) {
	*seen_error = TRUE;
	return LIB7_void;
    }
    if (header.byteOrder != ORDER) {
	if (BIGENDIAN_TO_HOST(header.byteOrder) != ORDER) {
	    *seen_error = TRUE;
	    return LIB7_void;
	}
	header.magic = BIGENDIAN_TO_HOST(header.magic);
	header.kind = BIGENDIAN_TO_HOST(header.kind);
	inBuf.needsSwap = TRUE;
    }
    if (header.magic != BLAST_MAGIC) {
	*seen_error = TRUE;
	return LIB7_void;
    }

    switch (header.kind) {
      case BLAST_IMAGE:
	if (ReadImage(lib7_state, &inBuf, &chunk) == FAILURE) {
	    *seen_error = TRUE;
	    return LIB7_void;
	}
	break;
      case BLAST_UNBOXED: {
	    lib7_blast_hdr_t	bhdr;
	    if (HeapIO_ReadBlock(&inBuf, &bhdr, sizeof(bhdr)) == FAILURE) {
	        *seen_error = TRUE;
	        return LIB7_void;
	    }
	    else
		chunk = bhdr.rootChunk;
	} break;
      default:
	*seen_error = TRUE;
	return LIB7_void;
    }

    return chunk;

} /* end of BlastIn */


/* ReadImage:
 */
static status_t ReadImage (lib7_state_t *lib7_state, inbuf_t *bp, lib7_val_t *chunkRef)
{
    lib7_blast_hdr_t	blastHdr;
    lib7_val_t		*externs;
    heap_arena_hdr_t	*arenaHdrs[NUM_CHUNK_KINDS], *arenaHdrsBuf;
    int			arenaHdrsSize, i;
    gen_t		*gen1 = lib7_state->lib7_heap->gen[0];

    if ((HeapIO_ReadBlock(bp, &blastHdr, sizeof(blastHdr)) == FAILURE)
    || (blastHdr.numArenas > NUM_ARENAS)
    || (blastHdr.numBOKinds > NUM_BIGCHUNK_KINDS))
	return FAILURE;

  /* read the externals table */
    externs = HeapIO_ReadExterns (bp);

  /* read the arena headers. */
    arenaHdrsSize = (blastHdr.numArenas + blastHdr.numBOKinds)
			* sizeof(heap_arena_hdr_t);
    arenaHdrsBuf = (heap_arena_hdr_t *) MALLOC (arenaHdrsSize);
    if (HeapIO_ReadBlock (bp, arenaHdrsBuf, arenaHdrsSize) == FAILURE) {
	FREE (arenaHdrsBuf);
	return FAILURE;
    }
    for (i = 0;  i < NUM_CHUNK_KINDS;  i++)
	arenaHdrs[i] = NULL;
    for (i = 0;  i < blastHdr.numArenas;  i++) {
	heap_arena_hdr_t	*p = &(arenaHdrsBuf[i]);
	arenaHdrs[p->chunkKind] = p;
    }
    /** DO BIG CHUNK HEADERS TOO **/

    /* Check the heap to see if there is
     * enough free space in the 1st generation:
     */
    {
	Addr_t	allocSzB = lib7_state->lib7_heap->allocSzB;
	bool_t	needsGC = FALSE;

	for (i = 0;  i < NUM_ARENAS;  i++) {
	    arena_t	*ap = gen1->arena[i];
	    if ((arenaHdrs[i] != NULL) && ((! isACTIVE(ap))
	    || (AVAIL_SPACE(ap) < arenaHdrs[i]->info.o.sizeB + allocSzB))) {
		needsGC = TRUE;
		ap->reqSizeB = arenaHdrs[i]->info.o.sizeB;
	    }
	}
	if (needsGC) {
	    if (bp->nbytes <= 0) {
		collect_garbage (lib7_state, 1);
	    } else {
	        /* The garbage collection may cause the buffer to move, so: */
		lib7_val_t	buffer = PTR_CtoLib7(bp->base);
		collect_garbage_with_extra_roots (lib7_state, 1, &buffer, NULL);
		if (buffer != PTR_CtoLib7(bp->base)) {
		    /* The buffer moved, so adjust the buffer pointers: */
		    Byte_t	*newBase = PTR_LIB7toC(Byte_t, buffer);
		    bp->buf = newBase + (bp->buf - bp->base);
		    bp->base = newBase;
		}
            }
	}
    }

    /** Read the blasted chunks **/
    {
	Addr_t	arenaBase[NUM_ARENAS];

	for (i = 0;  i < NUM_ARENAS;  i++) {
	    if (arenaHdrs[i] != NULL) {
	        arena_t	*ap = gen1->arena[i];
	        arenaBase[i] = (Addr_t)(ap->nextw);
	        HeapIO_ReadBlock (bp, (ap->nextw), arenaHdrs[i]->info.o.sizeB);
/*SayDebug ("[%2d] Read [%#x..%#x)\n", i+1, ap->nextw,*/
/*(Addr_t)(ap->nextw)+arenaHdrs[i]->info.o.sizeB);*/
	    }
	}

      /* adjust the pointers */
	for (i = 0;  i < NUM_ARENAS;  i++) {
	    if (arenaHdrs[i] != NULL) {
		arena_t	*ap = gen1->arena[i];
		if (i != STRING_INDEX) {
		    lib7_val_t	*p, *stop;
		    p = ap->nextw;
		    stop = (lib7_val_t *)((Addr_t)p + arenaHdrs[i]->info.o.sizeB);
		    while (p < stop) {
		        lib7_val_t	w = *p;
		        if (! isUNBOXED(w)) {
			    if (isEXTERNTAG(w)) {
			        w = externs[EXTERNID(w)];
			    }
			    else if (! isDESC(w)) {
/*SayDebug ("adjust (@%#x) %#x --> ", p, w);*/
			        w = PTR_CtoLib7(arenaBase[HIO_GET_ID(w)] + HIO_GET_OFFSET(w));
/*SayDebug ("%#x\n", w);*/
			    }
		            *p = w;
		        }
		        p++;
		    }
		    ap->nextw	=
		    ap->sweep_nextw	= stop;
	        }
	        else
		    ap->nextw = (lib7_val_t *)((Addr_t)(ap->nextw)
				+ arenaHdrs[i]->info.o.sizeB);
	    }
	} /* end of for */

      /* adjust the root chunk pointer */
	if (isEXTERNTAG(blastHdr.rootChunk))
	    *chunkRef = externs[EXTERNID(blastHdr.rootChunk)];
	else
	    *chunkRef = PTR_CtoLib7(
		    arenaBase[HIO_GET_ID(blastHdr.rootChunk)]
		    + HIO_GET_OFFSET(blastHdr.rootChunk));
/*SayDebug ("root = %#x, adjusted = %#x\n", blastHdr.rootChunk, *chunkRef);*/
    }

    FREE (arenaHdrsBuf);
    FREE (externs);

    return SUCCESS;

} /* end of ReadImage */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
