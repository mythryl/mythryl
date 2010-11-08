/* blast-gc.c
 *
 * This is the garbage collector for compacting a blasted chunk.
 *
 * NOTE: the extraction of literals could cause a space overflow.
 */

#include "../config.h"

#include <stdio.h>
#include "runtime-base.h"
#include "runtime-limits.h"
#include "runtime-state.h"
#include "runtime-values.h"
#include "memory.h"
#include "card-map.h"
#include "heap.h"
#include "tags.h"
#include "copy-loop.h"
#include "heap-monitor.h"
#include "runtime-timer.h"
#include "runtime-heap-image.h"
#include "blast-out.h"
#include "addr-hash.h"
#include "c-globals-table.h"
#include "runtime-heap.h"
#include "runtime-globals.h"


static bool_t	repairHeap;		/* this is TRUE, as long as it is cheaper */
					/* to repair the heap, than to complete */
					/* the collection */
static bool_t	finishGC;		/* this is TRUE, when we are finishing a */
					/* garbage collection after blasting. */
static int		maxCollectedGen;	/* the oldest generation being collected */
static lib7_val_t	*savedTop		/* save to-space top pointers */
		    [MAX_NUM_GENS][NUM_ARENAS];
static export_table_t *ExportTable;		/* the table of exported symbols */
static addr_table_t	*EmbChunkTable;		/* the table of embedded chunk references */

/* typedef struct repair repair_t; */  /* in heap.h */
struct repair {
    lib7_val_t	*loc;			/* the location to repair */
    lib7_val_t	val;			/* the old value */
};

/* record a location in a given arena for repair */
#define NOTE_REPAIR(ap, location, value)	{	\
	arena_t	*__ap = (ap);				\
	if (repairHeap) {				\
	    repair_t	*__rp = __ap->repairList - 1;	\
	    if ((lib7_val_t *)__rp > __ap->nextw) {	\
		__rp->loc = (location);			\
		__rp->val = (value);			\
		__ap->repairList = __rp;		\
	    }						\
	    else					\
		repairHeap = FALSE;			\
	}						\
    }

/* local routines */
static void BlastGC_RepairHeap (lib7_state_t *lib7_state, int maxGen);
static void BlastGC_FinishGC (lib7_state_t *lib7_state, int maxGen);
static void BlastGC_Flip (heap_t *heap, int gen);
static status_t BlastGC_SweepToSpace (heap_t *heap, aid_t maxAid);
/*
static bool_t BlastGC_SweepToSpArrays (heap_t *heap, arena_t *tosp, card_map_t *cm);
*/
static lib7_val_t BlastGC_ForwardChunk (heap_t *heap, lib7_val_t chunk, aid_t id);
static bigchunk_desc_t *BlastGC_ForwardBigChunk (
	heap_t *heap, lib7_val_t *p, lib7_val_t chunk, aid_t aid);
static embchunk_info_t *EmbChunkLookup (addr_table_t *table, Addr_t addr, embchunk_kind_t kind);
static void BlastGC_AssignLits (Addr_t addr, void *_closure, void *_info);
static void BlastGC_ExtractLits (Addr_t addr, void *_closure, void *_info);

struct assignlits_clos {	/* the closure for BlastGC_AssignLits */
    Word_t	id;		  /* the heap image chunk index for */
				  /* embedded literals */
    Word_t	offset;		  /* the offset of the next literal */
};

struct extractlits_clos {	/* the closure for BlastGC_ExtractLits */
    writer_t	*wr;
    Word_t	offset;		  /* the offset of the next literal; this is */
				  /* used to align reals. */
};


/* check to see if we need to extend the number of flipped generations */
#define CHECK_GEN(heap, g)	{		\
	int	__g = (g);			\
	if (__g > maxCollectedGen)		\
	    BlastGC_Flip ((heap), __g);		\
    }

/* BlastGC_CheckWord:
 *
 * Check an Lib7 value for external references, etc.
 */
#define BlastGC_CheckWord(heap, bibop, p, maxAid, seen_error) {			\
	lib7_val_t	__w = *(p);						\
/*SayDebug ("CheckWord @ %#x --> %#x: ", p, __w);*/\
	if (isBOXED(__w)) {							\
	    aid_t	__aid = ADDR_TO_PAGEID(bibop, __w);			\
	    if (isUNMAPPED(__aid)) {					\
	      /* an external reference */					\
/*SayDebug ("external reference\n");*/\
		if ((! finishGC) && (ExportCSymbol(ExportTable, __w) == LIB7_void))	\
		    (seen_error) = TRUE;						\
	    }									\
	    else if (IS_BIGCHUNK_AID(__aid))					\
/*{SayDebug ("big-chunk\n");*/\
		BlastGC_ForwardBigChunk(heap, p, __w, __aid);			\
/*}*/\
	    else if (IS_FROM_SPACE(__aid, maxAid))				\
/*{SayDebug ("regular chunk\n");*/\
		*(p) = BlastGC_ForwardChunk(heap, __w, __aid);			\
/*}*/\
	}									\
/*else SayDebug ("unboxed \n");*/\
    }


/* BlastGC:
 *
 */
blast_res_t BlastGC (lib7_state_t *lib7_state, lib7_val_t *root, int gen)
{
    heap_t	*heap = lib7_state->lib7_heap;
    bibop_t	bibop = BIBOP;
    blast_res_t	result;
    bool_t	seen_error = FALSE;

  /* Allocates the export and embedded chunk tables */
    ExportTable = NewExportTable();
    EmbChunkTable = MakeAddrTable(LOG_BYTES_PER_WORD, 64);

    result.exportTable	= ExportTable;
    result.embchunkTable	= EmbChunkTable;

  /* Initialize, by flipping the generations upto the one including the chunk */
    repairHeap = TRUE;
    finishGC = FALSE;
    maxCollectedGen = 0;
    BlastGC_Flip (heap, gen);

  /* Scan the chunk root */
    BlastGC_CheckWord (heap, bibop, root, AID_MAX, seen_error);
    if (seen_error) {
	result.error = TRUE;
	return result;
    }

  /* Sweep to-space */
    if (BlastGC_SweepToSpace(heap, AID_MAX) == FAILURE) {
	result.error = TRUE;
	return result;
    }

    result.error	= FALSE;
    result.needsRepair	= repairHeap;
    result.maxGen	= maxCollectedGen;

    return result;

} /* end of BlastGC. */


/* BlastGC_AssignLitAddresses:
 *
 * Assign relocation addresses to the embedded literals that are going to be
 * extracted.  The arguments to this are the blast result (containing the
 * embedded literal table), the ID of the heap image chunk that the string
 * literals are to be stored in, and the starting offset in that chunk.
 * This returns the address immediately following the last embedded literal.
 *
 * NOTE: this code will break if the size of the string space, plus embedded
 * literals exceeds 16Mb.
 */
Addr_t BlastGC_AssignLitAddresses (blast_res_t *res, int id, Addr_t offset)
{
    struct assignlits_clos closure;

    closure.offset = offset;
    closure.id = id;
    AddrTableApply (EmbChunkTable, &closure, BlastGC_AssignLits);

    return closure.offset;

} /* end of BlastGC_AssignLitAddresses */


/* BlastGC_BlastLits:
 *
 * Blast out the embedded literals.
 */
void BlastGC_BlastLits (writer_t *wr)
{
    struct extractlits_clos closure;

    closure.wr = wr;
    closure.offset = 0;
    AddrTableApply (EmbChunkTable, &closure, BlastGC_ExtractLits);

} /* end of BlastGC_BlastLits */


/* BlastGC_FinishUp:
 *
 * Finish up the blast-out operation.  This means either repairing the heap,
 * or completing the GC.
 */
void BlastGC_FinishUp (lib7_state_t *lib7_state, blast_res_t *res)
{
    if (res->needsRepair)
	BlastGC_RepairHeap (lib7_state, res->maxGen);
    else
	BlastGC_FinishGC (lib7_state, res->maxGen);

    FreeExportTable (ExportTable);
    FreeAddrTable (EmbChunkTable, TRUE);

} /* BlastGC_FinishUp */

/* BlastGC_RepairHeap:
 */
static void BlastGC_RepairHeap (lib7_state_t *lib7_state, int maxGen)
{
    heap_t	*heap = lib7_state->lib7_heap;
    int		i, j;

#ifdef VERBOSE
SayDebug ("Repairing blast GC (maxGen = %d of %d)\n", maxGen, heap->numGens);
#endif
    for (i = 0;  i < maxGen;  i++) {
	gen_t		*gen = heap->gen[i];

#define REPAIR(INDEX)	{						\
	arena_t		*__ap = gen->arena[INDEX];			\
	if (isACTIVE(__ap)) {						\
	    repair_t	*__stop, *__rp;					\
	    __stop = (repair_t *)(__ap->tospTop);			\
	    for (__rp = __ap->repairList;  __rp < __stop;  __rp++) {	\
		lib7_val_t	*__p = __rp->loc;			\
		if (INDEX != PAIR_INDEX)					\
		    __p[-1] = FOLLOW_FWDCHUNK(__p)[-1];			\
		__p[0] = __rp->val;					\
	    }								\
	}								\
    } /* end of REPAIR */

      /* repair the arenas */
	REPAIR(RECORD_INDEX);
	REPAIR(PAIR_INDEX);
	REPAIR(STRING_INDEX);
	REPAIR(ARRAY_INDEX);

      /* free the to-space chunk, and reset the BIBOP marks */
	for (j = 0;  j < NUM_ARENAS;  j++) {
	    arena_t	*ap = gen->arena[j];
	    if (isACTIVE(ap)) {
	      /* un-flip the spaces; note that FreeGeneration needs the from-space
	       * information.
	       */
		lib7_val_t	*tmpBase = ap->tospBase;
		Addr_t		tmpSizeB = ap->tospSizeB;
		lib7_val_t	*tmpTop = ap->tospTop;
		ap->nextw	=
		ap->sweep_nextw = ap->frspTop;
		ap->tospBase	= ap->frspBase;
		ap->frspBase	= tmpBase;
		ap->tospSizeB	= ap->frspSizeB;
		ap->frspSizeB	= tmpSizeB;
		ap->tospTop	= savedTop[i][j];
		ap->frspTop	= tmpTop;
	    }
	} /* end of for */
      /* free the to-space memory chunk */
	{
	    heap_chunk_t	*tmpChunk = gen->fromChunk;
	    gen->fromChunk = gen->toChunk;
	    gen->toChunk = tmpChunk;
	    FreeGeneration (heap, i);
	}
    } /* end of for */

} /* end of BlastGC_RepairHeap */


/* BlastGC_FinishGC:
 *
 * Complete the partial garbage collection.
 */
static void BlastGC_FinishGC (lib7_state_t *lib7_state, int maxGen)
{
    heap_t	*heap = lib7_state->lib7_heap;
    bibop_t	bibop = BIBOP;
    bool_t	dummy = FALSE;
    int		i, j;
    aid_t	maxAid;

#ifdef VERBOSE
SayDebug ("Completing blast GC (maxGen = %d of %d)\n", maxGen, heap->numGens);
#endif
    finishGC = TRUE;
    maxAid = MAKE_MAX_AID(maxGen);

  /* allocate new dirty vectors for the flipped generations */
    for (i = 0;  i < maxGen;  i++) {
	gen_t	*gen = heap->gen[i];
	if (isACTIVE(gen->arena[ARRAY_INDEX]))
	    NewDirtyVector(gen);
    }

  /* collect the roots */
#define CheckRoot(p)	{					\
	lib7_val_t	*__p = (p);				\
	BlastGC_CheckWord (heap, bibop, __p, maxAid, dummy);	\
    }

    for (i = 0;  i < NumCRoots;  i++)
	CheckRoot(CRoots[i]);

    CheckRoot(&(lib7_state->lib7_argument));
    CheckRoot(&(lib7_state->lib7_fate));
    CheckRoot(&(lib7_state->lib7_closure));
    CheckRoot(&(lib7_state->lib7_link_register));
    CheckRoot(&(lib7_state->lib7_program_counter));
    CheckRoot(&(lib7_state->lib7_exception_fate));
    CheckRoot(&(lib7_state->lib7_current_thread));
    CheckRoot(&(lib7_state->lib7_calleeSave[0]));
    CheckRoot(&(lib7_state->lib7_calleeSave[1]));
    CheckRoot(&(lib7_state->lib7_calleeSave[2]));

  /* sweep the dirty pages of generations over maxGen */
    for (i = maxGen; i < heap->numGens;  i++) {
	gen_t	*gen = heap->gen[i];
	if (isACTIVE(gen->arena[ARRAY_INDEX])) {
	    card_map_t	*cm = gen->dirty;
	    if (cm != NULL) {
		lib7_val_t	*maxSweep = gen->arena[ARRAY_INDEX]->sweep_nextw;
		int		card;
#if (!defined(BIT_CARDS) && defined(TOSPACE_ID))
		FOR_DIRTY_CARD (cm, maxGen, card, {
		    lib7_val_t	*p = (cm->baseAddr + (card*CARD_SZW));
		    lib7_val_t	*q = p + CARD_SZW;
		    int		mark = i+1;
		    if (q > maxSweep)
		      /* don't sweep above the allocation high-water mark */
			q = maxSweep;
		    for (;  p < q;  p++) {
			lib7_val_t	w = *p;
			if (isBOXED(w)) {
			    aid_t	aid = ADDR_TO_PAGEID(bibop, w);
			    int		targetGen;
			    if (IS_FROM_SPACE(aid, maxAid)) {
			      /* this is a from-space chunk */
			        if (IS_BIGCHUNK_AID(aid)) {
				    bigchunk_desc_t	*dp;
				    dp = BlastGC_ForwardBigChunk (heap, p, w, aid);
				    targetGen = dp->gen;
			        }
			        else {
				    *p =
				    w = BlastGC_ForwardChunk(heap, w, aid);
				    aid = ADDR_TO_PAGEID(bibop, w);
				    if (IS_TOSPACE_AID(aid))
				        targetGen = TOSPACE_GEN(aid);
				    else
				        targetGen = EXTRACT_GEN(aid);
			        }
			        if (targetGen < mark)
				    mark = targetGen;
			    }
		        }
		    } /* end of for */
		  /* re-mark the card */
		    ASSERT(cm->map[card] <= mark);
		    if (mark <= i)
			cm->map[card] = mark;
		    else if (i == maxGen)
			cm->map[card] = CARD_CLEAN;
		});
#elif (!defined(BIT_CARDS))
		FOR_DIRTY_CARD (cm, maxGen, card, {
		    lib7_val_t	*p = (cm->baseAddr + (card*CARD_SZW));
		    lib7_val_t	*q = p + CARD_SZW;
		    int		mark = i+1;
		    if (q > maxSweep)
		      /* don't sweep above the allocation high-water mark */
			q = maxSweep;
		    for (;  p < q;  p++) {
			lib7_val_t	w = *p;
			if (isBOXED(w)) {
			    aid_t	aid = ADDR_TO_PAGEID(bibop, w);
			    int		targetGen;
			    if (IS_FROM_SPACE(aid, maxAid)) {
			      /* this is a from-space chunk */
			        if (IS_BIGCHUNK_AID(aid)) {
				    bigchunk_desc_t	*dp;
				    dp = BlastGC_ForwardBigChunk (heap, p, w, aid);
				    targetGen = dp->gen;
			        }
			        else {
				    *p =
				    w = BlastGC_ForwardChunk(heap, w, aid);
				    targetGen = EXTRACT_GEN(ADDR_TO_PAGEID(bibop, w));
			        }
			        if (targetGen < mark)
				    mark = targetGen;
			    }
		        }
		    } /* end of for */
		  /* re-mark the card */
		    ASSERT(cm->map[card] <= mark);
		    if (mark <= i)
			cm->map[card] = mark;
		    else if (i == maxGen)
			cm->map[card] = CARD_CLEAN;
		});
#else
  /* BIT_CARDS */
#endif
	    }
	}
    }

  /* sweep to-space */
    BlastGC_SweepToSpace (heap, maxAid);

  /* Scan the rw_vector spaces of the flipped generations, marking dirty pages */
    for (i = 1;  i < maxGen;  i++) {
	gen_t		*gen = heap->gen[i];
	arena_t		*ap = gen->arena[ARRAY_INDEX];
	if (isACTIVE(ap)) {
	    card_map_t	*cm = gen->dirty;
	    int		card;
	    lib7_val_t	*p, *stop, w;

	    p = ap->tospBase;
	    card = 0;
	    while (p < ap->nextw) {
		int	mark = i+1;
		stop = (lib7_val_t *)(((Addr_t)p + CARD_SZB) & ~(CARD_SZB - 1));
		if (stop > ap->nextw)
		    stop = ap->nextw;
		while (p < stop) {
		    if (isBOXED(w = *p++)) {
			aid_t	aid = ADDR_TO_PAGEID(bibop, w);
			int	targetGen;

			if (IS_BIGCHUNK_AID(aid)) {
			    bigchunk_desc_t	*dp = BO_GetDesc(w);
			    targetGen = dp->gen;
			}
			else
			    targetGen = EXTRACT_GEN(aid);
			if (targetGen < mark) {
			    mark = targetGen;
			    if (mark == 1) {
				p = stop;
				break;  /* nothing dirtier than 1st generation */
			    }
			}
		    }
		}
		if (mark <= i)
		    cm->map[card] = mark;
		else
		    cm->map[card] = CARD_CLEAN;
		card++;
	    }
	}
    }

  /* reclaim space */
    for (i = 0;  i < maxGen;  i++) {
	FreeGeneration (heap, i);
#ifdef TOSPACE_ID
	for (j = 0;  j < NUM_ARENAS;  j++) {
	    arena_t		*ap = heap->gen[i]->arena[j];
	    if (isACTIVE(ap))
		MarkRegion (bibop, ap->tospBase, ap->tospSizeB, ap->id);
	}
#endif
    }

  /* remember the top of to-space in the collected generations */
    for (i = 0;  i < maxGen;  i++) {
	gen_t *g = heap->gen[i];
	if (i == heap->numGens-1) {
	  /* the oldest generation has only "young" chunks */
	    for (j = 0;  j < NUM_ARENAS;  j++) {
		if (isACTIVE(g->arena[j]))
		    g->arena[j]->oldTop = g->arena[j]->tospBase;
		else
		    g->arena[j]->oldTop = NULL;
	    }
	}
	else {
	    for (j = 0;  j < NUM_ARENAS;  j++) {
		if (isACTIVE(g->arena[j]))
		    g->arena[j]->oldTop = g->arena[j]->nextw;
		else
		    g->arena[j]->oldTop = NULL;
	    }
	}
    }

    HeapMon_UpdateHeap (heap, maxSweptGen);

#ifdef GC_STATS
  /* Count the number of forwarded bytes */
    for (i = 0;  i < maxGen;  i++) {
	for (j = 0;  j < NUM_ARENAS;  j++) {
	    arena_t	*ap = heap->gen[i]->arena[j];
	    if (isACTIVE(ap)) {
		CNTR_INCR(&(heap->numCopied[i][j]), ap->nextw - ap->tospBase);
	    }
	}
    }
#endif

} /* end of BlastGC_FinishGC */


/* BlastGC_Flip:
 *
 * Flip additional generations from maxCollectedGen+1 .. gen.  We allocate
 * a to-space that is the same size as the existing from-space.
 */
static void BlastGC_Flip (heap_t *heap, int gen)
{
    int		i, j;
    Addr_t	newSz;

    for (i = maxCollectedGen;  i < gen;  i++) {
	gen_t	*g = heap->gen[i];
	for (j = 0;  j < NUM_ARENAS;  j++) {
	    arena_t	*ap = g->arena[j];
	    if (isACTIVE(ap)) {
		ASSERT ((j == STRING_INDEX) || (ap->nextw == ap->sweep_nextw));
	        savedTop[i][j] = ap->tospTop;
		FLIP_ARENA(ap);
		newSz = (Addr_t)(ap->frspTop) - (Addr_t)(ap->frspBase);
		if (i == 0)
		  /* need to guarantee space for future minor collections */
		    newSz += heap->allocSzB;
		if (j == PAIR_INDEX)
		    newSz += 2*WORD_SZB;
		ap->tospSizeB = RND_HEAP_CHUNK_SZB(newSz);
	    }
	}
	g->fromChunk = g->toChunk;
#ifdef VERBOSE
SayDebug ("New Generation %d:\n", i+1);
#endif
	if (NewGeneration(g) == FAILURE)
	    Die ("unable to allocate to-space for generation %d\n", i+1);
     /* initialize the repair lists */
	for (j = 0;  j < NUM_ARENAS;  j++) {
	    arena_t	*ap = g->arena[j];
#ifdef VERBOSE
if (isACTIVE(ap)) SayDebug ("  %#x:  [%#x, %#x)\n", ap->id, ap->tospBase, ap->tospTop);
#endif
	    if (isACTIVE(ap))
		ap->repairList = (repair_t *)(ap->tospTop);
	}
    }

    maxCollectedGen = gen;

} /* end of BlastGC_Flip */

/* BlastGC_SweepToSpace:
 * Sweep the to-space arenas.  Because there are few references forward in time, we
 * try to completely scavenge a younger generation before moving on to the
 * next oldest.
 */
static status_t BlastGC_SweepToSpace (heap_t *heap, aid_t maxAid)
{
    int		i;
    bool_t	swept;
    bibop_t	bibop = BIBOP;
    bool_t	seen_error = FALSE;

#define SweepToSpArena(gen, index)	{					\
	arena_t	    *__ap = (gen)->arena[(index)];				\
	if (isACTIVE(__ap)) {							\
	    lib7_val_t    *__p, *__q;						\
	    __p = __ap->sweep_nextw;						\
	    if (__p < __ap->nextw) {						\
		swept = TRUE;							\
		do {								\
		    for (__q = __ap->nextw;  __p < __q;  __p++) {		\
			BlastGC_CheckWord(heap, bibop, __p, maxAid, seen_error);	\
		    }								\
		} while (__q != __ap->nextw);					\
		__ap->sweep_nextw = __q;					\
	    }									\
	}									\
    } /* SweepToSpArena */

    do {
	swept = FALSE;
	for (i = 0;  i < maxCollectedGen;  i++) {
	    gen_t	*gen = heap->gen[i];

	  /* Sweep the record and pair arenas */
	    SweepToSpArena(gen, RECORD_INDEX);
	    SweepToSpArena(gen, PAIR_INDEX);
	    SweepToSpArena(gen, ARRAY_INDEX);
	}
    } while (swept && (!seen_error));

    return (seen_error ? FAILURE : SUCCESS);

} /* end of SweepToSpace */


/* BlastGC_ForwardChunk:
 *
 * Forward an chunk.
 */
static lib7_val_t BlastGC_ForwardChunk (heap_t *heap, lib7_val_t v, aid_t id)
{
    lib7_val_t	*chunk = PTR_LIB7toC(lib7_val_t, v);
    int		gen = EXTRACT_GEN(id);
    lib7_val_t	*new_chunk;
    lib7_val_t	desc;
    Word_t	len;
    arena_t	*arena;

    if (! finishGC)
	CHECK_GEN(heap, gen);

    switch (EXTRACT_CHUNKC(id)) {
      case CHUNKC_record: {
	desc = chunk[-1];
	switch (GET_TAG(desc)) {
	  case DTAG_ro_vec_hdr:
	  case DTAG_rw_vec_hdr:
	    len = 2;
	    break;
	  case DTAG_forward:
	  /* This chunk has already been forwarded */
	    return PTR_CtoLib7(FOLLOW_FWDCHUNK(chunk));
	  default:
	    len = GET_LEN(desc);
	}
	arena = heap->gen[gen-1]->arena[RECORD_INDEX];
      } break;

      case CHUNKC_pair: {
	lib7_val_t	w;

	w = chunk[0];
	if (isDESC(w))
	    return PTR_CtoLib7(FOLLOW_FWDPAIR(w, chunk));
	else {
	  /* forward the pair */
	    arena = heap->gen[gen-1]->arena[PAIR_INDEX];
	    new_chunk = arena->nextw;
	    arena->nextw += 2;
	    new_chunk[0] = w;
	    new_chunk[1] = chunk[1];
	  /* setup the forward pointer in the old pair */
	    NOTE_REPAIR(arena, chunk, w);
	    chunk[0] =  MAKE_PAIR_FP(new_chunk);
	    return PTR_CtoLib7(new_chunk);
	}
      } break;

      case CHUNKC_string: {
	arena = heap->gen[gen-1]->arena[STRING_INDEX];
	desc = chunk[-1];
	switch (GET_TAG(desc)) {
	  case DTAG_forward:
	    return PTR_CtoLib7(FOLLOW_FWDCHUNK(chunk));
	  case DTAG_raw32:
	    len = GET_LEN(desc);
	    break;
	  case DTAG_raw64:
	    len = GET_LEN(desc);
#ifdef ALIGN_REALDS
#  ifdef CHECK_HEAP
	    if (((Addr_t)arena->nextw & WORD_SZB) == 0) {
		*(arena->nextw) = (lib7_val_t)0;
		arena->nextw++;
	    }
#  else
	    arena->nextw = (lib7_val_t *)(((Addr_t)arena->nextw) | WORD_SZB);
#  endif
#endif
	    break;
	  default:
	    Die ("bad string tag %d, chunk = %#x, desc = %#x",
		GET_TAG(desc), chunk, desc);
	}
      } break;

      case CHUNKC_array: {
	desc = chunk[-1];
	switch (GET_TAG(desc)) {
	  case DTAG_forward:
	  /* This chunk has already been forwarded */
	    return PTR_CtoLib7(FOLLOW_FWDCHUNK(chunk));
	  case DTAG_rw_vec_data:
	    len = GET_LEN(desc);
	    break;
	  case DTAG_special:
	  /* we are conservative here, and never nullify special chunks */
	    len = 1;
	    break;
	  default:
	    Die ("bad rw_vector tag %d, chunk = %#x, desc = %#x",
		GET_TAG(desc), chunk, desc);
	} /* end of switch */
	arena = heap->gen[gen-1]->arena[ARRAY_INDEX];
      } break;

      case CHUNKC_bigchunk:
      default:
	Die("BlastGC_ForwardChunk: unknown chunk ilk %d @ %#x",
	    EXTRACT_CHUNKC(id), chunk);
    } /* end of switch */

  /* Allocate and initialize a to-space copy of the chunk */
    new_chunk = arena->nextw;
    arena->nextw += (len + 1);
    *new_chunk++ = desc;
    COPYLOOP(chunk, new_chunk, len);

  /* set up the forward pointer, and return the new chunk. */
    NOTE_REPAIR(arena, chunk, *chunk);
    chunk[-1] = DESC_forwarded;
    chunk[0] = (lib7_val_t)(Addr_t)new_chunk;
    return PTR_CtoLib7(new_chunk);

} /* end of BlastGC_ForwardChunk */


/* BlastGC_ForwardBigChunk:
 *
 * Forward a big-chunk chunk, where id is the BIBOP entry for chunk, and return
 * the big-chunk descriptor.
 * NOTE: we do not ``promote'' big-chunks here, because are not reclaimed
 * when completing th collection.
 */
static bigchunk_desc_t *BlastGC_ForwardBigChunk (
    heap_t	    *heap,
    lib7_val_t	    *p,
    lib7_val_t	    chunk,
    aid_t	    aid)
{
    int		    i;
    bigchunk_region_t *region;
    bigchunk_desc_t   *dp;
    embchunk_info_t   *codeInfo;

    for (i = BIBOP_ADDR_TO_INDEX(chunk);  !BO_IS_HDR(aid);  aid = BIBOP[--i])
	continue;
    region = (bigchunk_region_t *)BIBOP_INDEX_TO_ADDR(i);
    dp = ADDR_TO_BODESC(region, chunk);

    if (! finishGC) {
	CHECK_GEN(heap, dp->gen);
	codeInfo = EmbChunkLookup (EmbChunkTable, dp->chunk, UNUSED_CODE);
	codeInfo->kind = USED_CODE;
    }

    return dp;

} /* end of BlastGC_ForwardBigChunk */


/* EmbChunkLookup:
 */
static embchunk_info_t *EmbChunkLookup (addr_table_t *table, Addr_t addr, embchunk_kind_t kind)
{
    embchunk_info_t	*p = FindEmbChunk(table, addr);

    if (p == NULL) {
	p		= NEW_CHUNK(embchunk_info_t);
	p->kind		= kind;
	p->codeChunk	= NULL;
	AddrTableInsert(table, addr, p);
    }

    ASSERT(kind == p->kind);

    return p;

} /* end of EmbChunkLookup */

/* BlastGC_AssignLits:
 *
 * Calculate the location of the extracted literal strings in the blasted
 * image, and record their addresses.  This function is passed as an argument
 * to AddrTableApply; its second argument is its "closure," and its third
 * argument is the embedded chunk info.
 */
static void BlastGC_AssignLits (Addr_t addr, void *_closure, void *_info)
{
#ifdef XXX
    struct assignlits_clos *closure = (struct assignlits_clos *) _closure;
    embchunk_info_t	*info = (embchunk_info_t *) _info;
    int			chunkSzB;

    switch (info->kind) {
      case UNUSED_CODE:
      case USED_CODE:
	info->relAddr = (lib7_val_t)0;
	return;
      case EMB_STRING: {
	    int		nChars = CHUNK_LEN(PTR_CtoLib7(addr));
	    int		nWords = BYTES_TO_WORDS(nChars);
	    if ((nChars != 0) && ((nChars & 0x3) == 0))
	        nWords++;
	    chunkSzB = nWords * WORD_SZB;
	} break;
      case EMB_REALD:
	chunkSzB = CHUNK_LEN(PTR_CtoLib7(addr)) * REALD_SZB;
#ifdef ALIGN_REALDS
	closure->offset |= WORD_SZB;
#endif
	break;
    }

    if (info->codeChunk->kind == USED_CODE) {
      /* the containing code chunk is also being exported */
	info->relAddr = (lib7_val_t)0;
	return;
    }

    if (chunkSzB == 0) {
	info->relAddr = ExportCSymbol (ExportTable,
		(info->kind == EMB_STRING) ? LIB7_string0 : LIB7_realarray0);
    }
    else {
      /* assign a relocation address to the chunk, and bump the offset counter */
	closure->offset += WORD_SZB;  /* space for the descriptor */
	info->relAddr = HIO_TAG_PTR(closure->id, closure->offset);
	closure->offset += chunkSzB;
    }
#else
Die ("BlastGC_AssignLits");
#endif
} /* end of BlastGC_AssignLits */

/* BlastGC_ExtractLits:
 *
 * Extract the embedded literals that are in otherwise unreferenced code
 * blocks.  This function is passed as an argument to AddrTableApply; its
 * second argument is its "closure," and its third argument is the
 * embedded chunk info.
 */
static void BlastGC_ExtractLits (Addr_t addr, void *_closure, void *_info)
{
    struct extractlits_clos *closure = (struct extractlits_clos *) _closure;
    embchunk_info_t	*info = (embchunk_info_t *) _info;
    int			chunkSzB;

    if (info->relAddr == (lib7_val_t)0)
	return;

    switch (info->kind) {
      case EMB_STRING: {
	    int		nChars = CHUNK_LEN(PTR_CtoLib7(addr));
	    int		nWords = BYTES_TO_WORDS(nChars);
	    if ((nChars != 0) && ((nChars & 0x3) == 0))
	        nWords++;
	    chunkSzB = nWords * WORD_SZB;
	} break;
      case EMB_REALD:
	chunkSzB = CHUNK_LEN(PTR_CtoLib7(addr)) * REALD_SZB;
#ifdef ALIGN_REALDS
	if ((closure->offset & (REALD_SZB-1)) == 0) {
	    /* the descriptor would be 8-byte aligned, which means that the
	     * real number would not be, so add some padding.
	     */
	    WR_Put(closure->wr, 0);
	    closure->offset += 4;
	}
#endif
	break;
    }

    if (chunkSzB != 0) {
      /* extract the chunk into the blast buffer (including the descriptor) */
	WR_Write(closure->wr, (void *)(addr - WORD_SZB), chunkSzB + WORD_SZB);
	closure->offset += (chunkSzB + WORD_SZB);
    }

} /* end of BlastGC_ExtractLits */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
