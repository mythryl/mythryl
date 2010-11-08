/* export-heap.c
 *
 * Routines to export an Lib7 heap image.  The basic layout of the heap image is:
 *
 *   Header (Image header + Heap header)
 *   External reference table
 *   Lib7 state info
 *   Lib7 Heap:
 *     Big-chunk region descriptors
 *     Generation descriptors
 *     Heap image
 *
 *
 * Note that this will change once multiple VProcs are supported.
 */


/*                        "A good warrior is not bellicose,
 *                         A good fighter does not anger,
 *                         A good conqueror does not contest his enemy,
 *                         One who is good at using others puts himself below them."
 *
 *                                                          -Lao Tzu
 */


#include "../config.h"

#include "runtime-osdep.h"
#include "runtime-base.h"
#include "runtime-limits.h"
#include "memory.h"
#include "runtime-state.h"
#include "runtime-heap.h"
#include "runtime-globals.h"
#include "runtime-heap-image.h"
#include "heap.h"
#include "c-globals-table.h"
#include "writer.h"
#include "heap-io.h"
#include "heap-output.h"

#define isEXTERN(bibop, w)	(isBOXED(w) && (ADDR_TO_PAGEID(bibop, w) == AID_UNMAPPED))
#define isEXTERNTAG(w)		(isDESC(w) && (GET_TAG(w) == DTAG_extern))

/* local routines */
static status_t write_heap_image_to_file (lib7_state_t *lib7_state, int kind, FILE *file);
static export_table_t *build_export_table (heap_t *heap);
static status_t write_heap (writer_t *wr, heap_t *heap);
static void repair_heap (export_table_t *table, heap_t *heap);


status_t   ExportHeapImage   (   lib7_state_t*   lib7_state,
                                 FILE*            file
                             )
{
    return write_heap_image_to_file (lib7_state, EXPORT_HEAP_IMAGE, file);

} 


status_t   ExportFnImage   (   lib7_state_t*   lib7_state,
                               lib7_val_t      funct,
                               FILE*            file
                           )
{
    /* Zero out the saved parts of the Lib7 state,
     * and use the standard argument register to
     * hold the exported function closure:
     */
    lib7_state->lib7_argument		= funct;
    lib7_state->lib7_fate		= LIB7_void;
    lib7_state->lib7_closure		= LIB7_void;
    lib7_state->lib7_link_register	= LIB7_void;
    lib7_state->lib7_exception_fate	= LIB7_void;
    lib7_state->lib7_current_thread	= LIB7_void;	/* ??? */
    lib7_state->lib7_calleeSave[0]	= LIB7_void;
    lib7_state->lib7_calleeSave[1]	= LIB7_void;
    lib7_state->lib7_calleeSave[2]	= LIB7_void;

    return write_heap_image_to_file (lib7_state, EXPORT_FN_IMAGE, file);
}



static status_t   write_heap_image_to_file   (   lib7_state_t*   lib7_state,
                                                 int              kind,
                                                 FILE*            file
                                             )
{
    heap_t*	heap = lib7_state->lib7_heap;

/*  gen_t*	oldestGen = heap->gen[heap->numGens-1]; */

    status_t	    status = SUCCESS;
    export_table_t* export_table;
    writer_t*	    wr;

    #define SAVE_REG(dst, src)	{				\
		lib7_val_t	__src = (src);			\
		if (isEXTERN(BIBOP, __src))			\
		    __src = ExportCSymbol(export_table, __src);	\
		(dst) = __src;					\
	    }

    if ((wr = WR_OpenFile(file)) == NULL)
	return FAILURE;

    /* Shed any and all garbage. */
    collect_garbage (lib7_state, 0);  /* minor collection */
    collect_garbage (lib7_state, MAX_NGENS);

    export_table = build_export_table( heap );

    {   lib7_heap_hdr_t	heapHdr;

	heapHdr.numVProcs	= 1;
	heapHdr.numGens		= heap->numGens;
	heapHdr.numArenas	= NUM_ARENAS;
	heapHdr.numBOKinds	= NUM_BIGCHUNK_KINDS;
	heapHdr.numBORegions	= heap->numBORegions;
	heapHdr.cacheGen	= heap->cacheGen;
	heapHdr.allocSzB	= heap->allocSzB / MAX_NUM_PROCS;

	SAVE_REG(heapHdr.pervasiveStruct, *PTR_LIB7toC(lib7_val_t, PervasiveStruct));
	SAVE_REG(heapHdr.runtimeCompileUnit, runtimeCompileUnit);
#ifdef ASM_MATH
	SAVE_REG(heapHdr.mathVec, MathVec);
#else
	heapHdr.mathVec = LIB7_void;
#endif

	HeapIO_WriteImageHeader(wr, kind);
	WR_Write(wr, &heapHdr, sizeof(heapHdr));
	if (WR_Error(wr)) {
	    WR_Free(wr);
	    return FAILURE;
	}
    }

    /* Export the Lib7 state info: */
    {   lib7_vproc_image_t	image;

        /* Save the live registers */
	SAVE_REG( image.sigHandler,	DEREF(Lib7SignalHandler)	);
	SAVE_REG( image.stdArg,		lib7_state->lib7_argument	);
	SAVE_REG( image.stdCont,	lib7_state->lib7_fate		);
	SAVE_REG( image.stdClos,	lib7_state->lib7_closure	);
	SAVE_REG( image.pc,		lib7_state->lib7_program_counter);
	SAVE_REG( image.exception_fate,	lib7_state->lib7_exception_fate	);
	SAVE_REG( image.current_thread,	lib7_state->lib7_current_thread	);
	SAVE_REG( image.calleeSave[0],	lib7_state->lib7_calleeSave[0] 	);
	SAVE_REG( image.calleeSave[1],	lib7_state->lib7_calleeSave[1] 	);
	SAVE_REG( image.calleeSave[2],	lib7_state->lib7_calleeSave[2] 	);

	if (HeapIO_WriteExterns(wr, export_table) == FAILURE) {
	    status = FAILURE;
	    goto done;
	}

	WR_Write(wr, &image, sizeof(image));
	if (WR_Error(wr)) {
	    status = FAILURE;
	    goto done;         /* say it isn't so, Sam. */
	}
    }

    /* Write out the heap image: */
    if (write_heap(wr, heap) == FAILURE) {
	status = FAILURE;
    }

done:;
    if (kind != EXPORT_FN_IMAGE) {
	repair_heap( export_table, heap );
    }

    WR_Free(wr);

    return status;
}


static export_table_t*   build_export_table   (heap_t* heap)
{
    /* Scan the heap looking for exported 
     * symbols and return an export table:
     */

    export_table_t*	table = NewExportTable();
    bibop_t		bibop = BIBOP;

    #define PatchArena(index)	{					\
		arena_t		*__ap = heap->gen[i]->arena[(index)];	\
		lib7_val_t	*__p, *__q;				\
		bool_t		needsRepair = FALSE;			\
		__p = __ap->tospBase;					\
		__q = __ap->nextw;					\
		while (__p < __q) {					\
		    lib7_val_t	__w = *__p;				\
		    if (isEXTERN(bibop, __w)) {				\
			*__p = ExportCSymbol(table, __w);		\
			needsRepair = TRUE;				\
		    }							\
		    __p++;						\
		}							\
		__ap->needsRepair = needsRepair;			\
	    }

    /* Scan the record, pair and rw_vector regions
     * for references to external symbols:
     */
    int  i;
    for (i = 0;  i < heap->numGens;  i++) {
	PatchArena( RECORD_INDEX );
	PatchArena( PAIR_INDEX   );
	PatchArena( ARRAY_INDEX  );
    }

    return table;
}


static status_t   write_heap   (   writer_t*  wr,
                                   heap_t*    heap
                               )
{
    heap_arena_hdr_t*	p;
    heap_arena_hdr_t*	arenaHdrs;
    bigchunk_desc_t*	bdp;
    int			arenaHdrsSize;
    int			pagesize;
    long		offset;
    int			i, j;

    pagesize = GETPAGESIZE();

    /* Write the big-chunk region descriptors: */
    {
	int			size;
	bo_region_info_t	*header;
	bigchunk_region_t		*rp;

#ifdef BO_DEBUG
SayDebug("%d bigchunk regions\n", heap->numBORegions);
#endif
	size = heap->numBORegions * sizeof(bo_region_info_t);
	header = (bo_region_info_t *) MALLOC (size);
	for (rp = heap->bigRegions, i = 0;  rp != NULL;  rp = rp->next, i++) {
#ifdef BO_DEBUG
PrintRegionMap(rp);
#endif
	    header[i].baseAddr	= HEAP_CHUNK_BASE( rp->heap_chunk );
	    header[i].firstPage	= rp->firstPage;
	    header[i].sizeB	= HEAP_CHUNK_SZB( rp->heap_chunk );
	}

	WR_Write(wr, header, size);
	if (WR_Error(wr)) {
	    FREE (header);
	    return FAILURE;
	}

	FREE(header);
    }

    /* Initialize the arena headers: */
    arenaHdrsSize = heap->numGens * (NUM_CHUNK_KINDS * sizeof(heap_arena_hdr_t));
    arenaHdrs = (heap_arena_hdr_t *) MALLOC (arenaHdrsSize);
    offset = WR_Tell(wr) + arenaHdrsSize;
    offset = ROUNDUP(offset, pagesize);
    for (p = arenaHdrs, i = 0;  i < heap->numGens;  i++) {
	for (j = 0;  j < NUM_ARENAS;  j++, p++) {
	    arena_t		*ap = heap->gen[i]->arena[j];
	    p->gen		    = i;
	    p->chunkKind		    = j;
	    p->info.o.baseAddr	    = (Addr_t)(ap->tospBase);
	    p->info.o.sizeB	    = (Addr_t)(ap->nextw) - p->info.o.baseAddr;
	    p->info.o.roundedSzB    = ROUNDUP(p->info.o.sizeB, pagesize);
	    p->offset		    = (Unsigned32_t)offset;
	    offset		    += p->info.o.roundedSzB;
	}
	for (j = 0;  j < NUM_BIGCHUNK_KINDS;  j++, p++) {
	    int			nChunks, nBOPages;
	    bdp = heap->gen[i]->bigChunks[j];
	    for (nChunks = nBOPages = 0;  bdp != NULL;  bdp = bdp->next) {
		nChunks++;
		nBOPages += (BO_ROUNDED_SZB(bdp) >> BIGCHUNK_PAGE_SHIFT);
	    }
	    p->gen		    = i;
	    p->chunkKind		    = j;
	    p->info.bo.numBigChunks   = nChunks;
	    p->info.bo.numBOPages   = nBOPages;
	    p->offset		    = (Unsigned32_t)offset;
	    offset		    += ((nChunks * sizeof(bigchunk_hdr_t))
					+ (nBOPages << BIGCHUNK_PAGE_SHIFT));
	}
    }

    /* Write out the arena headers: */
    WR_Write(wr, arenaHdrs, arenaHdrsSize);
    if (WR_Error(wr)) {
	FREE (arenaHdrs);
	return FAILURE;
    }

    /* Write out the arenas: */
    for (p = arenaHdrs, i = 0;  i < heap->numGens;  i++) {
	for (j = 0;  j < NUM_ARENAS;  j++) {
	    if (GCMessages) {
		SayDebug("write %d,%d: %d bytes [%#x..%#x) @ %#x\n",
		    i+1, j, p->info.o.sizeB,
		    p->info.o.baseAddr, p->info.o.baseAddr+p->info.o.sizeB,
		    p->offset);
	    }
	    if (p->info.o.sizeB > 0) {
		WR_Seek(wr, p->offset);
		WR_Write(wr, (void *)(p->info.o.baseAddr), p->info.o.sizeB);
		if (WR_Error(wr)) {
		    FREE (arenaHdrs);
		    return FAILURE;
		}
	    }
	    p++;
	}
	for (j = 0;  j < NUM_BIGCHUNK_KINDS;  j++) {
	    int			hdrSizeB;
	    bigchunk_hdr_t	*header, *q;

	    if (p->info.bo.numBigChunks > 0) {

		hdrSizeB = p->info.bo.numBigChunks * sizeof(bigchunk_hdr_t);
		header = (bigchunk_hdr_t *) MALLOC (hdrSizeB);
		if (GCMessages) {
		    SayDebug("write %d,%d: %d big chunks (%d pages) @ %#x\n",
			i+1, j, p->info.bo.numBigChunks, p->info.bo.numBOPages,
			p->offset);
		}

	        /* Initialize the big-chunk headers: */
		q = header;
		for (bdp = heap->gen[i]->bigChunks[j];  bdp != NULL;  bdp = bdp->next) {
		    q->gen		= bdp->gen;
		    q->chunkKind		= j;
		    q->baseAddr	= (Addr_t)(bdp->chunk);
		    q->sizeB		= bdp->sizeB;
		    q++;
		}

	        /* Write the big-chunk headers: */
		WR_Write (wr, header, hdrSizeB);
		if (WR_Error(wr)) {
		    FREE (header);
		    FREE (arenaHdrs);
		    return FAILURE;
		}

	        /* Write the big-chunks: */
		for (bdp = heap->gen[i]->bigChunks[j];  bdp != NULL;  bdp = bdp->next) {
		    WR_Write(wr, (char *)(bdp->chunk), BO_ROUNDED_SZB(bdp));
		    if (WR_Error(wr)) {
			FREE (header);
			FREE (arenaHdrs);
			return FAILURE;
		    }
		}
		FREE (header);
	    }
	    p++;
	}
    }

    FREE (arenaHdrs);

    return SUCCESS;
}



static void   repair_heap   (   export_table_t* table,
                                heap_t*         heap
                            )
{
    #define RepairArena(index)	{					\
	    arena_t		*__ap = heap->gen[i]->arena[(index)];	\
	    if (__ap->needsRepair) {					\
		lib7_val_t	*__p, *__q;				\
		__p = __ap->tospBase;					\
		__q = __ap->nextw;					\
		while (__p < __q) {					\
		    lib7_val_t	__w = *__p;				\
		    if (isEXTERNTAG(__w)) {				\
			*__p = AddrOfCSymbol(table, __w);			\
		    }							\
		    __p++;						\
		}							\
	    }								\
	    __ap->needsRepair = FALSE;					\
	}


    /* Repair the in-memory heap: */
    int  i;
    for (i = 0;  i < heap->numGens;  i++) {

	RepairArena( RECORD_INDEX );
	RepairArena( PAIR_INDEX   );
	RepairArena( ARRAY_INDEX  );
    }

    FreeExportTable (table);
}

/* COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

