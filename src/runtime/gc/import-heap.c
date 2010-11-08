/* import-heap.c
 *
 * Routines to import an Lib7 heap image.
 */

#include "../config.h"

#include <stdio.h>
#include <string.h>
#include "runtime-base.h"
#include "machine-id.h"
#include "memory.h"
#include "cache-flush.h"
#include "runtime-state.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "runtime-globals.h"
#include "card-map.h"
#include "heap.h"
#include "runtime-heap-image.h"
#include "c-globals-table.h"
#include "addr-hash.h"
#include "heap-input.h"
#include "heap-io.h"

#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#ifdef DEBUG
static void print_region_map (bo_region_reloc_t *r)
{
    bo_reloc_t		*dp, *dq;
    int			i;

    SayDebug ("region @%#x: |", r->firstPage);
    for (i = 0, dq = r->chunkMap[0];  i < r->nPages;  i++) {
	dp = r->chunkMap[i];
	if (dp != dq) {
	    SayDebug ("|");
	    dq = dp;
	}
	if (dp == NULL)
	    SayDebug ("_");
	else
	    SayDebug ("X");
    }
    SayDebug ("|\n");

}
#endif



static void read_heap (inbuf_t*          bp,
                       lib7_heap_hdr_t* header,
                       lib7_state_t*    lib7_state,
                       lib7_val_t*      externs );

static bigchunk_desc_t*   AllocBODesc (bigchunk_desc_t*, bigchunk_hdr_t*, bo_region_reloc_t* );

static void repair_heap (
	heap_t *, aid_t *, Addr_t [MAX_NUM_GENS][NUM_ARENAS],
	addr_table_t *, lib7_val_t *);

static lib7_val_t repair_word (
	lib7_val_t w, aid_t *oldBIBOP, Addr_t addrOffset[MAX_NUM_GENS][NUM_ARENAS],
	addr_table_t *boRegionTable, lib7_val_t *externs);

/*
static int RepairBORef (aid_t *bibop, aid_t id, lib7_val_t *ref, lib7_val_t oldChunk);
*/

static bo_reloc_t *address_to_relocation_info (aid_t *, addr_table_t *, aid_t, Addr_t);

#define READ(bp,chunk)	HeapIO_ReadBlock(bp, &(chunk), sizeof(chunk))


/* ImportHeapImage:
 */
lib7_state_t *ImportHeapImage (const char *fname, heap_params_t *params)
{
    lib7_state_t		*lib7_state;
    lib7_image_hdr_t	imHdr;
    lib7_heap_hdr_t	heapHdr;
    lib7_val_t		*externs;
    lib7_vproc_image_t	image;
    inbuf_t		inBuf;

    if (fname != NULL) {
      /* Resolve the name of the image.
       *  If the file exists use it, otherwise try the
       * pathname with the machine ID as an extension.
       */
      if ((inBuf.file = fopen(fname, "rb"))) {

	if (verbosity > 0)   say("loading %s ", fname);

      } else {

	if ((inBuf.file = fopen(fname, "rb"))) {

	  if (verbosity > 0)   say("loading %s ", fname);

	} else {
	    Die ("unable to open heap image \"%s\"\n", fname);
        }
      }

      inBuf.needsSwap = FALSE;
      inBuf.buf	    = NULL;
      inBuf.nbytes    = 0;
    } else {
      /* fname == NULL, so try to find an in-core heap image */
#if defined(DLOPEN) && !defined(OPSYS_WIN32)
      void *lib = dlopen (NULL, RTLD_LAZY);
      void *vimg, *vimglenptr;
      if ((vimg = dlsym(lib,HEAP_IMAGE_SYMBOL)) == NULL)
	Die("no in-core heap image found\n");
      if ((vimglenptr = dlsym(lib,HEAP_IMAGE_LEN_SYMBOL)) == NULL)
	Die("unable to find length of in-core heap image\n");

      inBuf.file = NULL;
      inBuf.needsSwap = FALSE;
      inBuf.base = vimg;
      inBuf.buf = inBuf.base;
      inBuf.nbytes = *(long*)vimglenptr;
#else
      Die("in-core heap images not implemented\n");
#endif
    }

    READ(&inBuf, imHdr);
    if (imHdr.byteOrder != ORDER)
	Die ("incorrect byte order in heap image\n");
    if (imHdr.magic != IMAGE_MAGIC)
	Die ("bad magic number (%#x) in heap image\n", imHdr.magic);
    if ((imHdr.kind != EXPORT_HEAP_IMAGE) && (imHdr.kind != EXPORT_FN_IMAGE))
	Die ("bad image kind (%d) in heap image\n", imHdr.kind);
    READ(&inBuf, heapHdr);

    /* Check for command-line overrides of heap parameters: */
    if (params->allocSz == 0) params->allocSz = heapHdr.allocSzB;
    if (params->numGens < heapHdr.numGens) params->numGens = heapHdr.numGens;
    if (params->cacheGen < 0) params->cacheGen = heapHdr.cacheGen;

    lib7_state = AllocLib7state (FALSE, params);

    /* Get the run-time pointers into the heap: */
    *PTR_LIB7toC(lib7_val_t, PervasiveStruct) = heapHdr.pervasiveStruct;
    runtimeCompileUnit = heapHdr.runtimeCompileUnit;
#ifdef ASM_MATH
    MathVec = heapHdr.mathVec;
#endif

    /* Read the externals table: */
    externs = HeapIO_ReadExterns (&inBuf);

    /* Read and initialize the Lib7 state info: */
    READ(&inBuf, image);
    if (imHdr.kind == EXPORT_HEAP_IMAGE) {
      /* Load the live registers */
	ASSIGN(Lib7SignalHandler, image.sigHandler);
	lib7_state->lib7_argument	= image.stdArg;
	lib7_state->lib7_fate		= image.stdCont;
	lib7_state->lib7_closure	= image.stdClos;
	lib7_state->lib7_program_counter= image.pc;
	lib7_state->lib7_exception_fate	= image.exception_fate;
	lib7_state->lib7_current_thread	= image.current_thread;
	lib7_state->lib7_calleeSave[0]	= image.calleeSave[0];
	lib7_state->lib7_calleeSave[1]	= image.calleeSave[1];
	lib7_state->lib7_calleeSave[2]	= image.calleeSave[2];
        /* Read the Lib7 heap */
	read_heap (&inBuf, &heapHdr, lib7_state, externs);
      /* GC message are on by default for interactive images */
      /* GCMessages = TRUE; */
    }
    else {  /* EXPORT_FN_IMAGE */

	lib7_val_t	funct, cmdName, args;

        /* Restore the signal handler: */
	ASSIGN(Lib7SignalHandler, image.sigHandler);

        /* Read the Lib7 heap: */
	lib7_state->lib7_argument		= image.stdArg;
	read_heap (&inBuf, &heapHdr, lib7_state, externs);

        /* Initialize the calling context (taken from ApplyLib7Fn) */
	funct				= lib7_state->lib7_argument;
	lib7_state->lib7_exception_fate	= PTR_CtoLib7(handle_v+1);
	lib7_state->lib7_current_thread	= LIB7_void;
	lib7_state->lib7_fate		= PTR_CtoLib7(return_c);
	lib7_state->lib7_closure	= funct;
	lib7_state->lib7_program_counter=
	lib7_state->lib7_link_register	= GET_CODE_ADDR(funct);

        /* Set up the arguments to the imported function */
	cmdName = LIB7_CString(lib7_state, Lib7CommandName);
	args = LIB7_CStringList (lib7_state, commandline_arguments);
	REC_ALLOC2(lib7_state, lib7_state->lib7_argument, cmdName, args);
/*
SayDebug("arg = %#x : [%#x, %#x]\n", lib7_state->lib7_argument, REC_SEL(lib7_state->lib7_argument, 0), REC_SEL(lib7_state->lib7_argument, 1));
*/
        /* GC message are off by default for spawn_to_disk images */
	GCMessages = FALSE;
    }

    FREE (externs);

    if (inBuf.file)   fclose (inBuf.file);

    if (verbosity > 0)   say(" done\n");

    return lib7_state;
}                                /* ImportHeapImage */



static void   read_heap   (   inbuf_t*            bp,
                             lib7_heap_hdr_t*   header,
                             lib7_state_t*      lib7_state,
                             lib7_val_t*        externs
                         )
{
    heap_t*		heap = lib7_state->lib7_heap;
    heap_arena_hdr_t	*arenaHdrs, *p, *q;
    int			arenaHdrsSize;
    int			i, j, k;
    long		prevSzB[NUM_ARENAS], size;
    bibop_t		oldBIBOP;
    Addr_t		addrOffset[MAX_NUM_GENS][NUM_ARENAS];
    bo_region_reloc_t	*boRelocInfo;
    addr_table_t		*boRegionTable;

    /* Allocate a BIBOP for the imported
     * heap image's address space:
     */
#ifdef TWO_LEVEL_MAP
#  error two level map not supported
#else
    oldBIBOP = NEW_VEC (aid_t, BIBOP_SZ);
#endif

    /* Read in the big-chunk region descriptors
     * for the old address space:
     */
    {
	int		  size;
	bo_region_info_t* boRgnHdr;

	boRegionTable = MakeAddrTable(BIBOP_SHIFT+1, header->numBORegions);
	size = header->numBORegions * sizeof(bo_region_info_t);
	boRgnHdr = (bo_region_info_t *) MALLOC (size);
	HeapIO_ReadBlock (bp, boRgnHdr, size);

	boRelocInfo = NEW_VEC(bo_region_reloc_t, header->numBORegions);

	for (i = 0;  i < header->numBORegions;  i++) {
	    MarkRegion(oldBIBOP,
		(lib7_val_t *)(boRgnHdr[i].baseAddr),
		RND_HEAP_CHUNK_SZB(boRgnHdr[i].sizeB),
		AID_BIGCHUNK(1)
            );
	    oldBIBOP[BIBOP_ADDR_TO_INDEX(boRgnHdr[i].baseAddr)] = AID_BIGCHUNK_HDR(MAX_NUM_GENS);
	    boRelocInfo[i].firstPage = boRgnHdr[i].firstPage;

	    boRelocInfo[i].nPages
                =
                (boRgnHdr[i].sizeB - (boRgnHdr[i].firstPage - boRgnHdr[i].baseAddr))
                >>
                BIGCHUNK_PAGE_SHIFT;

	    boRelocInfo[i].chunkMap = NEW_VEC(bo_reloc_t *, boRelocInfo[i].nPages);

	    for (j = 0;  j < boRelocInfo[i].nPages;  j++) {
		boRelocInfo[i].chunkMap[j] = NULL;
            } 
	    AddrTableInsert (boRegionTable, boRgnHdr[i].baseAddr, &(boRelocInfo[i]));
	}
	FREE (boRgnHdr);
    }

    /* Read the arena headers: */
    arenaHdrsSize = header->numGens * NUM_CHUNK_KINDS * sizeof(heap_arena_hdr_t);
    arenaHdrs = (heap_arena_hdr_t *) MALLOC (arenaHdrsSize);
    HeapIO_ReadBlock (bp, arenaHdrs, arenaHdrsSize);

    for (i = 0;  i < NUM_ARENAS;  i++) {
	prevSzB[i] = heap->allocSzB;
    }

    /* Allocate the arenas and read in the heap image: */
    for (p = arenaHdrs, i = 0;  i < header->numGens;  i++) {
	gen_t	*gen = heap->gen[i];

	/* Compute the space required for this generation,
	 * and mark the oldBIBOP to reflect the old address space:
	 */
	for (q = p, j = 0;  j < NUM_ARENAS;  j++) {
	    MarkRegion (oldBIBOP,
		(lib7_val_t *)(q->info.o.baseAddr),
		RND_HEAP_CHUNK_SZB(q->info.o.sizeB),
		gen->arena[j]->id);
	    size = q->info.o.sizeB + prevSzB[j];
	    if ((j == PAIR_INDEX) && (size > 0))
		size += 2*WORD_SZB;
	    gen->arena[j]->tospSizeB = RND_HEAP_CHUNK_SZB(size);
	    prevSzB[j] = q->info.o.sizeB;
	    q++;
	}

        /* Allocate space for the generation: */
	if (NewGeneration(gen) == FAILURE) {
	    Die ("unable to allocated space for generation %d\n", i+1);
        } 
	if (isACTIVE(gen->arena[ARRAY_INDEX])) {
	    NewDirtyVector (gen);
        }

	/* Read in the arenas for this generation
	 * and initialize the address offset table:
	 */
	for (j = 0;  j < NUM_ARENAS;  j++) {

	    arena_t* ap = gen->arena[j];

	    if (p->info.o.sizeB > 0) {

		addrOffset[i][j] = (Addr_t)(ap->tospBase) - (Addr_t)(p->info.o.baseAddr);

		HeapIO_Seek (bp, (long)(p->offset));
		HeapIO_ReadBlock(bp, (ap->tospBase), p->info.o.sizeB);

		ap->nextw  = (lib7_val_t *)((Addr_t)(ap->tospBase) + p->info.o.sizeB);
		ap->oldTop = ap->tospBase;

	    } else if (isACTIVE(ap)) {

		ap->oldTop = ap->tospBase;
	    }

	    if (verbosity > 0)   say(".");

	    p++;
	}

        /* Read in the big-chunk arenas: */
	for (j = 0;  j < NUM_BIGCHUNK_KINDS;  j++) {
	    Addr_t		totSizeB;
	    bigchunk_desc_t	*freeChunk, *bdp;
	    bigchunk_region_t	*freeRegion;
	    bigchunk_hdr_t	*boHdrs;
	    int			boHdrSizeB, index;
	    bo_region_reloc_t   *region;

	    if (p->info.bo.numBOPages > 0) {
		totSizeB = p->info.bo.numBOPages << BIGCHUNK_PAGE_SHIFT;
		freeChunk = BO_AllocRegion (heap, totSizeB);
		freeRegion = freeChunk->region;
		freeRegion->minGen = i;
		MarkRegion (BIBOP, (lib7_val_t *)freeRegion,
		    HEAP_CHUNK_SZB( freeRegion->heap_chunk ), AID_BIGCHUNK(i));
		BIBOP[BIBOP_ADDR_TO_INDEX(freeRegion)] = AID_BIGCHUNK_HDR(i);

	        /* Read in the big-chunk headers */
		boHdrSizeB = p->info.bo.numBigChunks * sizeof(bigchunk_hdr_t);
		boHdrs = (bigchunk_hdr_t *) MALLOC (boHdrSizeB);
		HeapIO_ReadBlock (bp, boHdrs, boHdrSizeB);

	        /* Read in the big-chunks: */
		HeapIO_ReadBlock (bp, (void *)(freeChunk->chunk), totSizeB);
		if (j == CODE_INDEX) {
		    FlushICache ((void *)(freeChunk->chunk), totSizeB);
		}

	        /* Set up the big-chunk descriptors 
                 * and per-chunk relocation info:
                 */
		for (k = 0;  k < p->info.bo.numBigChunks;  k++) {
		  /* find the region relocation info for the chunk's region in
		   * the exported heap.
		   */
		    for (index = BIBOP_ADDR_TO_INDEX(boHdrs[k].baseAddr);
			!BO_IS_HDR(oldBIBOP[index]);
			index--)
			continue;
		    region = LookupBORegion (boRegionTable, index);

		    /* Allocate the big-chunk descriptor for
		     * the chunk and link it into the list
                     * of big-chunks for its generation.
		     */
		    bdp = AllocBODesc (freeChunk, &(boHdrs[k]), region);
		    bdp->next = gen->bigChunks[j];
		    gen->bigChunks[j] = bdp;
		    ASSERT(bdp->gen == i+1);

		    if (show_code_chunk_comments && (j == CODE_INDEX)) {

		        /* Dump the comment string of the code chunk: */
			char* namestring;
			if ((namestring = BO_GetCodeChunkTag(bdp))) {
			    SayDebug ("[%6d bytes] %s\n", bdp->sizeB, namestring);
                        }
		    }
		}

		if (freeChunk != bdp) {
		    /* There was some extra space left in the region: */
		    ADD_BODESC(heap->freeBigChunks, freeChunk);
		}

		FREE (boHdrs);
	    }

	    if (verbosity > 0)   say(".");

	    p++;
	}
    }

    repair_heap (heap, oldBIBOP, addrOffset, boRegionTable, externs);

    /* Adjust the run-time globals
     * that point into the heap:
     */
    *PTR_LIB7toC(lib7_val_t, PervasiveStruct) = repair_word (
	*PTR_LIB7toC(lib7_val_t, PervasiveStruct),
	oldBIBOP, addrOffset, boRegionTable, externs);

    runtimeCompileUnit = repair_word( runtimeCompileUnit, oldBIBOP, addrOffset, boRegionTable, externs );

#ifdef ASM_MATH
    MathVec = repair_word (MathVec, oldBIBOP, addrOffset, boRegionTable, externs);
#endif

    /* Adjust the Lib7 registers to the new address space */
    ASSIGN(Lib7SignalHandler, repair_word (
	DEREF(Lib7SignalHandler), oldBIBOP, addrOffset, boRegionTable, externs)
    );

    lib7_state->lib7_argument = repair_word (
        lib7_state->lib7_argument, oldBIBOP, addrOffset, boRegionTable, externs
    );

    lib7_state->lib7_fate = repair_word (
        lib7_state->lib7_fate, oldBIBOP, addrOffset, boRegionTable, externs
    );

    lib7_state->lib7_closure = repair_word (
        lib7_state->lib7_closure, oldBIBOP, addrOffset, boRegionTable, externs
    );

    lib7_state->lib7_program_counter = repair_word (
        lib7_state->lib7_program_counter, oldBIBOP, addrOffset, boRegionTable, externs
    );

    lib7_state->lib7_link_register = repair_word (
        lib7_state->lib7_link_register, oldBIBOP, addrOffset, boRegionTable, externs
    );

    lib7_state->lib7_exception_fate = repair_word (
        lib7_state->lib7_exception_fate, oldBIBOP, addrOffset, boRegionTable, externs
    );

    lib7_state->lib7_current_thread = repair_word (
        lib7_state->lib7_current_thread, oldBIBOP, addrOffset, boRegionTable, externs
    );

    lib7_state->lib7_calleeSave[0] = repair_word (
        lib7_state->lib7_calleeSave[0], oldBIBOP, addrOffset, boRegionTable, externs
    );

    lib7_state->lib7_calleeSave[1] = repair_word (
        lib7_state->lib7_calleeSave[1], oldBIBOP, addrOffset, boRegionTable, externs
    );

    lib7_state->lib7_calleeSave[2] = repair_word (
        lib7_state->lib7_calleeSave[2], oldBIBOP, addrOffset, boRegionTable, externs
    );

    /* Release storage: */
    for (i = 0; i < header->numBORegions;  i++) {
	bo_reloc_t	*p;
	for (p = NULL, j = 0;  j < boRelocInfo[i].nPages;  j++) {
	    if ((boRelocInfo[i].chunkMap[j] != NULL)
	    && (boRelocInfo[i].chunkMap[j] != p)) {
		FREE (boRelocInfo[i].chunkMap[j]);
		p = boRelocInfo[i].chunkMap[j];
	    }
	}
    }
    FreeAddrTable (boRegionTable, FALSE);
    FREE (boRelocInfo);
    FREE (arenaHdrs);
    FREE (oldBIBOP);

    /* Reset the sweep_nextw pointers: */
    for (i = 0;  i < heap->numGens;  i++) {
	gen_t	*gen = heap->gen[i];
	for (j = 0;  j < NUM_ARENAS;  j++) {
	    arena_t		*ap = gen->arena[j];
	    if (isACTIVE(ap))
		ap->sweep_nextw = ap->nextw;
	}
    }

}                                                       /* read_heap. */



static bigchunk_desc_t*   AllocBODesc (   bigchunk_desc_t*  free,
                                          bigchunk_hdr_t*    chunkHdr,
                                          bo_region_reloc_t* oldRegion
) {
    bigchunk_desc_t  * newChunk;
    bo_reloc_t	     * relocInfo;

    int	 i;
    int	 firstPage;

    int totSzB = ROUNDUP(chunkHdr->sizeB, BIGCHUNK_PAGE_SZB);
    int npages = (totSzB >> BIGCHUNK_PAGE_SHIFT);

    bigchunk_region_t* region = free->region;

    if (free->sizeB == totSzB) {

        /* Allocate the whole
         * free area to the chunk:
         */
	newChunk = free;

    } else {

        /* Split the free chunk: */
	newChunk		= NEW_CHUNK(bigchunk_desc_t);
	newChunk->chunk	= free->chunk;
	newChunk->region	= region;
	free->chunk	= (Addr_t)(free->chunk) + totSzB;
	free->sizeB	-= totSzB;
	firstPage	= ADDR_TO_BOPAGE(region, newChunk->chunk);
	for (i = 0;  i < npages;  i++) {
	    region->chunkMap[firstPage+i] = newChunk;
        }
    }

    newChunk->sizeB	= chunkHdr->sizeB;
    newChunk->state	= BO_YOUNG;
    newChunk->gen		= chunkHdr->gen;
    newChunk->chunkc	= chunkHdr->chunkKind;
    region->nFree	-= npages;

    /* Set up the relocation info: */
    relocInfo = NEW_CHUNK(bo_reloc_t);
    relocInfo->oldAddr = chunkHdr->baseAddr;
    relocInfo->newChunk = newChunk;
    firstPage = ADDR_TO_BOPAGE(oldRegion, chunkHdr->baseAddr);
    for (i = 0;  i < npages;  i++) {
	oldRegion->chunkMap[firstPage+i] = relocInfo;
    }

    return newChunk;
}                                            /* AllocBODesc */



static void   repair_heap   (   heap_t*       heap,
			       aid_t*        oldBIBOP,
			       Addr_t        addrOffset  [ MAX_NUM_GENS ][ NUM_ARENAS ],
			       addr_table_t* boRegionTable,
			       lib7_val_t*  externs
) {
    /* Scan the heap, replacing external references with their addresses and
     * adjusting pointers.
     */

    int  i;
    for (i = 0;  i < heap->numGens;  i++) {
	gen_t	*gen = heap->gen[i];

#ifndef BIT_CARDS
#define MARK(cm, p, g)	MARK_CARD(cm, p, g)
#else
#define MARK(cm, p, g)	MARK_CARD(cm, p)
#endif

#define REPAIR_ARENA(index)	{						\
	    arena_t		*__ap = gen->arena[(index)];			\
	    lib7_val_t	*__p, *__q;						\
	    __p = __ap->tospBase;						\
	    __q = __ap->nextw;							\
	    while (__p < __q) {							\
		lib7_val_t	__w = *__p;					\
		int		__gg, __chunkc;					\
		if (isBOXED(__w)) {						\
		    Addr_t	__chunk = PTR_LIB7toADDR(__w);			\
		    aid_t	__aid = ADDR_TO_PAGEID(oldBIBOP, __chunk);	\
		    if (IS_BIGCHUNK_AID(__aid)) {					\
			bo_reloc_t	*__dp;					\
			__dp = address_to_relocation_info (oldBIBOP, boRegionTable,		\
				__aid, __chunk);					\
			*__p = PTR_CtoLib7((__chunk - __dp->oldAddr) 		\
				+ __dp->newChunk->chunk);				\
			__gg = __dp->newChunk->gen-1;				\
		    }								\
		    else {							\
			__gg = EXTRACT_GEN(__aid)-1;				\
			__chunkc = EXTRACT_CHUNKC(__aid)-1;				\
			*__p = PTR_CtoLib7(__chunk + addrOffset[__gg][__chunkc]);	\
		    }								\
		    if (((index) == ARRAY_INDEX) && (__gg < i)) {			\
			MARK(gen->dirty, __p, __gg+1);	/** **/			\
		    }								\
		}								\
		else if (isEXTERNTAG(__w)) {					\
		    *__p = externs[EXTERNID(__w)];				\
		}								\
		__p++;								\
	    }									\
	}

	REPAIR_ARENA( RECORD_INDEX );
	REPAIR_ARENA( PAIR_INDEX   );
	REPAIR_ARENA( ARRAY_INDEX  );
#undef REPAIR_ARENA
    }

}                                           /* repair_heap */



static lib7_val_t   repair_word   (   lib7_val_t   w,
				       aid_t*        oldBIBOP,
				       Addr_t        addrOffset  [ MAX_NUM_GENS ][ NUM_ARENAS ],
				       addr_table_t* boRegionTable,
				       lib7_val_t*  externs
) {
    if (isBOXED(w)) {
	Addr_t	chunk = PTR_LIB7toADDR(w);
	aid_t	aid = ADDR_TO_PAGEID(oldBIBOP, chunk);

	if (IS_BIGCHUNK_AID(aid)) {

	    bo_reloc_t* dp = address_to_relocation_info (oldBIBOP, boRegionTable, aid, chunk);

	    return PTR_CtoLib7((chunk - dp->oldAddr) + dp->newChunk->chunk);

	} else {

	    int	g = EXTRACT_GEN(aid)-1;
	    int	chunkc = EXTRACT_CHUNKC(aid)-1;
	    return PTR_CtoLib7(PTR_LIB7toC(char, w) + addrOffset[g][chunkc]);
	}

    } else if (isEXTERNTAG(w)) {
	return externs[EXTERNID(w)];
    } else {
	return w;
    }
}



static bo_reloc_t*   address_to_relocation_info   (   aid_t*          oldBIBOP, 
						       addr_table_t*   boRegionTable, 
						       aid_t           id, 
						       Addr_t          oldChunk
) {
    int		       index;
    bo_region_reloc_t* region;

    for (index = BIBOP_ADDR_TO_INDEX(oldChunk);  !BO_IS_HDR(id);  id = oldBIBOP[--index])
	continue;

    /* Find the old region descriptor: */
    region = LookupBORegion (boRegionTable, index);

    if (!region) {
	Die ("unable to map big-chunk @ %#x; index = %#x, id = %#x\n",
	    oldChunk, index, (unsigned)id);
    }

    return ADDR_TO_BODESC(region, oldChunk);
}


/* COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

