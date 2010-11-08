/* major-gc.c
 *
 * This is the regular garbage collector (for collecting the
 * generations).
 */

/*
###                         "Youth had been a habit of hers for so long,
###                          that she could not part with it."
###
###                                             -- Rudyard Kipling
 */


/* When we go to parallel garbage collection, would it
 * help to segregate all REF cells in a separate area,
 * as a special-case?  This would localize all heap
 * side effects to arrays, segregated REF cells and
 * maybe activation records. (Not sure how those are
 * currently handled.)
*/

#ifdef PAUSE_STATS		/* GC pause statistics are UNIX dependent */
#  include "runtime-unixdep.h"
#endif

#include "../config.h"

#include "runtime-base.h"
#include "runtime-limits.h"
#include "runtime-state.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "memory.h"
#include "card-map.h"
#include "heap.h"
#include "tags.h"
#include "copy-loop.h"
#include "heap-monitor.h"
#include "runtime-timer.h"
#include "gc-stats.h"

#ifdef GC_STATS
long		lastMinorGC = 0;
long		numUpdates = 0;
long		numBytesAlloc = 0;
long		numBytesCopied = 0;
#endif

#ifdef BO_REF_STATS
static long numBO1, numBO2, numBO3;
#define IFBO_COUNT1(aid)	{if (IS_BIGCHUNK_AID(aid)) numBO1++;}
#define BO2_COUNT		(numBO2)++
#define BO3_COUNT		(numBO3)++
#else
#define IFBO_COUNT1(aid)	{}
#define BO2_COUNT		{}
#define BO3_COUNT		{}
#endif

#ifdef COUNT_CARDS
#ifndef BIT_CARDS
static unsigned long cardCnt1[MAX_NUM_GENS], cardCnt2[MAX_NUM_GENS];
#define COUNT_CARD1(i)	(cardCnt1[i]++)
#define COUNT_CARD2(i)	(cardCnt2[i]++)
#else
static unsigned long cardCnt[MAX_NUM_GENS];
#define COUNT_CARD(i)	(cardCnt[i]++)
#endif
#else
#define COUNT_CARD(i)	{}
#define COUNT_CARD1(i)	{}
#define COUNT_CARD2(i)	{}
#endif


/** DEBUG **/
#ifdef  BO_DEBUG
static void ScanMem (Word_t *start, Word_t *stop, int gen, int chunkKind)
{
    bibop_t	    bibop = BIBOP;
    Word_t	    w;
    int		    index;
    aid_t	    aid;
    bigchunk_region_t *region;
    bigchunk_desc_t   *dp;

    while (start < stop) {
	w = *start;
	if (isBOXED(w)) {
	    int		index = BIBOP_ADDR_TO_INDEX(w);
	    aid_t	id = bibop[index];
	    switch (EXTRACT_CHUNKC(id)) {
	      case CHUNKC_bigchunk:
		while (!BO_IS_HDR(id)) {
		    id = bibop[--index];
		}
		region = (bigchunk_region_t *)BIBOP_INDEX_TO_ADDR(index);
		dp = ADDR_TO_BODESC(region, w);
		if (dp->state == BO_FREE) {
		    SayDebug ("** [%d/%d]: %#x --> %#x; unexpected free big-chunk\n",
			gen, chunkKind, start, w);
		}
		break;
	      case CHUNKC_record:
	      case CHUNKC_pair:
	      case CHUNKC_string:
	      case CHUNKC_array:
		break;
	      default:
		if (id != AID_UNMAPPED)
		    SayDebug ("** [%d/%d]: %#x --> %#x; strange chunk ilk %d\n",
			gen, chunkKind, start, w, EXTRACT_CHUNKC(id));
		break;
	    }
	}
	start++;
    }
}
#endif /** BO_DEBUG **/

/* local routines */
static void MajorGC_ScanRoots (
	lib7_state_t *lib7_state, heap_t *heap, lib7_val_t **roots, int maxCollectedGen);
static void MajorGC_SweepToSpace (heap_t *heap, int maxCollectedGen, int maxSweptGen);
static bool_t MajorGC_SweepToSpArrays (
	heap_t *heap, int maxGen, arena_t *tosp, card_map_t *cm);
static lib7_val_t MajorGC_ForwardChunk (
	heap_t *heap, aid_t maxAid, lib7_val_t chunk, aid_t id);
static bigchunk_desc_t *MajorGC_ForwardBigChunk (
	heap_t *heap, int maxGen, lib7_val_t chunk, aid_t id);
static lib7_val_t MajorGC_FwdSpecial (
	heap_t *heap, aid_t maxAid, lib7_val_t *chunk, aid_t id, lib7_val_t desc);
static void TrimHeap (heap_t *heap, int maxCollectedGen);

/* the symbolic names of the arenas */
char		*ArenaName[NUM_ARENAS+1] = {
	"new", "record", "pair", "string", "array"
    };
/* DEBUG */static char *StateName[] = {"FREE", "YOUNG", "FORWARD", "OLD", "PROMOTE"};

/* Check a word for a from-space reference */
#ifdef TOSPACE_ID
#define NO_GC_INLINE /* DEBUG */
#endif
#ifndef NO_GC_INLINE
#define MajorGC_CheckWord(heap,bibop,maxAid,p)	{			\
	lib7_val_t	__w = *(p);					\
	if (isBOXED(__w)) {						\
	    aid_t	__aid = ADDR_TO_PAGEID(bibop, __w);		\
IFBO_COUNT1(__aid);							\
	    if (IS_FROM_SPACE(__aid,maxAid)) {				\
		*(p) = MajorGC_ForwardChunk(heap, maxAid, __w, __aid);	\
	    }								\
        }								\
    }
#else
static void MajorGC_CheckWord (heap_t *heap, bibop_t bibop, aid_t maxAid, lib7_val_t *p)
{
    lib7_val_t	w = *(p);
    if (isBOXED(w)) {
	aid_t	arena_id = ADDR_TO_PAGEID(bibop, w);
IFBO_COUNT1(arena_id);							\
	if (IS_FROM_SPACE(arena_id, maxAid)) {
	    *(p) = MajorGC_ForwardChunk(heap, maxAid, w, arena_id);
	}
#ifdef TOSPACE_ID
	else if (IS_TOSPACE_AID(arena_id)) {
	    Die ("CheckWord: TOSPACE reference: %#x (%#x) --> %#x\n",
		p, ADDR_TO_PAGEID(bibop, p), w);
	}
#endif
    }
}
#endif


/* MajorGC:
 *
 * Do a garbage collection of (at least) the first 'level' generations.
 * By definition, level should be at least 1.
 */
void MajorGC (lib7_state_t *lib7_state, lib7_val_t **roots, int level)
{
    heap_t	*heap = lib7_state->lib7_heap;
    bibop_t	bibop = BIBOP;
    int		i, j;
    int		maxCollectedGen;	/* the oldest generation being collected */
    int		maxSweptGen;
#ifdef GC_STATS
    lib7_val_t	*tospTop[NUM_ARENAS]; /* for counting # of bytes forwarded */
#endif

#ifndef PAUSE_STATS	/* don't do timing when collecting pause data */
    start_garbage_collection_timer(lib7_state->lib7_vproc);
#endif
#ifdef BO_REF_STATS
numBO1 = numBO2 = numBO3 = 0;
#endif

  /* Flip to-space and from-space */
    maxCollectedGen = Flip (heap, level);
    if (maxCollectedGen < heap->numGens) {
	maxSweptGen = maxCollectedGen+1;
#ifdef GC_STATS
      /* Remember the top of to-space for maxSweptGen */
	for (i = 0;  i < NUM_ARENAS;  i++)
	    tospTop[i] = heap->gen[maxSweptGen-1]->arena[i]->nextw;
#endif /* GC_STATS */
    }
    else {
	maxSweptGen = maxCollectedGen;
    }
    NUM_GC_GENS(maxCollectedGen);	/* record pause info */

#ifdef VM_STATS
    ReportVM (lib7_state, maxCollectedGen);
#endif

#ifndef PAUSE_STATS	/* don't do messages when collecting pause data */
    if (GCMessages) {
	SayDebug ("GC #");
	for (i = heap->numGens-1;  i >= 0; i--) {
	    SayDebug ("%d.", heap->gen[i]->numGCs);
	}
	SayDebug ("%d:  ", heap->numMinorGCs);
    }
#endif

    HeapMon_StartGC (heap, maxCollectedGen);

    /* Scan the roots */
    MajorGC_ScanRoots (lib7_state, heap, roots, maxCollectedGen);

    /* Sweep to-space */
    MajorGC_SweepToSpace (heap, maxCollectedGen, maxSweptGen);

    /* Handle weak pointers */
    if (heap->weakList != NULL)
	ScanWeakPtrs (heap);

  /* reclaim from-space; we do this from oldest to youngest so that
   * we can promote big chunks.
   */
    for (i = maxCollectedGen;  i > 0;  i--) {
	gen_t		*gen = heap->gen[i-1], *promoteGen;
	int		forwardState, promoteState;

	FreeGeneration (heap, i-1);
#ifdef TOSPACE_ID
	for (j = 0;  j < NUM_ARENAS;  j++) {
	    arena_t	*ap = gen->arena[j];
	    if (isACTIVE(ap))
		MarkRegion (bibop, ap->tospBase, ap->tospSizeB, ap->id);
	}
#endif
      /* NOTE: there should never be any big-chunks in the oldest generation
       * with the BO_PROMOTE tag.
       */
	if (i != heap->numGens) {
	    promoteGen = heap->gen[i];
	    forwardState = BO_OLD;
	  /* the chunks promoted from generation i to generation i+1, when
	   * generation i+1 is also being collected, are "OLD", thus we need
	   * to mark the corresponding big chunks as old so that they do not
	   * get out of sync.  Since the oldest generation has only YOUNG
	   * chunks, we have to check for that case too.
	   */
	    if ((i == maxCollectedGen) || (i == heap->numGens-1))
		promoteState = BO_YOUNG;
	    else
		promoteState = BO_OLD;
	}
	else {
	  /* oldest generation chunks are promoted to the same generation */
	    promoteGen = heap->gen[i-1];
	    forwardState = BO_YOUNG; /* oldest gen has only YOUNG chunks */
	}
	for (j = 0;  j < NUM_BIGCHUNK_KINDS;  j++) {
	    bigchunk_desc_t   *dp, *dq, *forward, *promote;
	    promote = promoteGen->bigChunks[j];
	    forward = NULL;
	    for (dp = gen->bigChunks[j];  dp != NULL;  ) {
		dq = dp->next;
		ASSERT(dp->gen == i);
		switch (dp->state) {
		  case BO_YOUNG:
		  case BO_OLD:
		    BO_Free (heap, dp);
		    break;
		  case BO_FORWARD:
		    dp->state = forwardState;
		    dp->next = forward;
		    forward = dp;
		    break;
		  case BO_PROMOTE:
		    dp->state = promoteState;
		    dp->next = promote;
		    dp->gen++;
		    promote = dp;
		    break;
		  default:
		    Die ("strange bigchunk state %d @ %#x in generation %d\n",
			dp->state, dp, i);
		} /* end switch */
		dp = dq;
	    }
	    promoteGen->bigChunks[j] = promote; /* a nop for the oldest generation */
	    gen->bigChunks[j] = forward;
	}
    }
#ifdef BO_DEBUG
/** DEBUG **/
for (i = 0;  i < heap->numGens;  i++) {
gen_t	*gen = heap->gen[i];
ScanMem((Word_t *)(gen->arena[RECORD_INDEX]->tospBase), (Word_t *)(gen->arena[RECORD_INDEX]->nextw), i+1, RECORD_INDEX);
ScanMem((Word_t *)(gen->arena[PAIR_INDEX]->tospBase), (Word_t *)(gen->arena[PAIR_INDEX]->nextw), i+1, PAIR_INDEX);
ScanMem((Word_t *)(gen->arena[ARRAY_INDEX]->tospBase), (Word_t *)(gen->arena[ARRAY_INDEX]->nextw), i+1, ARRAY_INDEX);
}
/** DEBUG **/
#endif

  /* relabel BIBOP entries for big-chunk regions to reflect promotions */
    {
	bigchunk_region_t	*rp;
	bigchunk_desc_t	*dp;
	int		min;

	for (rp = heap->bigRegions;  rp != NULL;  rp = rp->next) {
	  /* if the minimum generation of the region is less than or equal
	   * to maxCollectedGen, then it is possible that it has increased
	   * as a result of promotions or freeing of chunks.
	   */
	    if (rp->minGen <= maxCollectedGen) {
		min = MAX_NUM_GENS;
		for (i = 0;  i < rp->nPages; ) {
		    dp = rp->chunkMap[i];
		    if ((! BO_IS_FREE(dp)) && (dp->gen < min))
			min = dp->gen;
		    i += BO_NUM_BOPAGES(dp);
		}
		if (rp->minGen != min) {
		    rp->minGen = min;
		    MarkRegion (bibop, (lib7_val_t *)rp, HEAP_CHUNK_SZB( rp->heap_chunk ),
			AID_BIGCHUNK(min));
		    bibop[BIBOP_ADDR_TO_INDEX(rp)] = AID_BIGCHUNK_HDR(min);
		}
	    }
	} /* end for */
    }

  /* remember the top of to-space in the collected generations */
    for (i = 0;  i < maxCollectedGen;  i++) {
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
    if (maxSweptGen != maxCollectedGen) {
	gen_t	*gen = heap->gen[maxSweptGen-1];
	for (j = 0;  j < NUM_ARENAS;  j++) {
	    CNTR_INCR(&(heap->numCopied[maxSweptGen-1][j]),
		gen->arena[j]->nextw - tospTop[j]);
	}
    }
    for (i = 0;  i < maxCollectedGen;  i++) {
	for (j = 0;  j < NUM_ARENAS;  j++) {
	    arena_t	*ap = heap->gen[i]->arena[j];
	    if (isACTIVE(ap)) {
		CNTR_INCR(&(heap->numCopied[i][j]), ap->nextw - tospTop[j]);
	    }
	}
    }
#endif

#ifdef BO_REF_STATS
SayDebug ("bigchunk stats: %d seen, %d lookups, %d forwarded\n",
numBO1, numBO2, numBO3);
#endif
#ifndef PAUSE_STATS	/* don't do timing when collecting pause data */
    if (GCMessages) {
	long	gcTime;
	stop_garbage_collection_timer (lib7_state->lib7_vproc, &gcTime);
	SayDebug (" (%d ms)\n", gcTime);
    }
    else
	stop_garbage_collection_timer (lib7_state->lib7_vproc, NULL);
#endif

#ifdef VM_STATS
    ReportVM (lib7_state, 0);
#endif

#ifdef CHECK_HEAP
    CheckHeap(heap, maxSweptGen);
#endif

    if (! UnlimitedHeap)
	TrimHeap (heap, maxCollectedGen);

} /* end of MajorGC. */


/* MajorGC_ScanRoots:
 */
static void MajorGC_ScanRoots (
    lib7_state_t	*lib7_state,
    heap_t	*heap,
    lib7_val_t	**roots,
    int		maxCollectedGen)
{
    bibop_t	bibop = BIBOP;
    aid_t	maxAid = MAKE_MAX_AID(maxCollectedGen);
    lib7_val_t	*rp;
    int		i;

    while ((rp = *roots++) != NULL) {
	MajorGC_CheckWord(heap, bibop, maxAid, rp);
    }

  /* Scan the dirty cards in the older generations */
    for (i = maxCollectedGen;  i < heap->numGens;  i++) {
	gen_t	    *gen = heap->gen[i];
#ifdef COUNT_CARDS
#ifndef BIT_CARDS
/*CARD*/cardCnt1[i]=cardCnt2[i]=0;
#else
/*CARD*/cardCnt[i]=0;
#endif
#endif
	if (isACTIVE(gen->arena[ARRAY_INDEX])) {
	    card_map_t	*cm = gen->dirty;
	    if (cm != NULL) {
		lib7_val_t    *maxSweep = gen->arena[ARRAY_INDEX]->sweep_nextw;
		int	    card;
#if (!defined(BIT_CARDS) && defined(TOSPACE_ID))
		FOR_DIRTY_CARD (cm, maxCollectedGen, card, {
		    lib7_val_t	*p = (cm->baseAddr + (card*CARD_SZW));
		    lib7_val_t	*q = p + CARD_SZW;
		    int		mark = i+1;
COUNT_CARD1(i);
		    if (q > maxSweep)
		      /* don't sweep above the allocation high-water mark */
			q = maxSweep;
		    for (;  p < q;  p++) {
			lib7_val_t	w = *p;
			if (isBOXED(w)) {
			    aid_t	aid = ADDR_TO_PAGEID(bibop, w);
			    int		targetGen;
IFBO_COUNT1(aid);
			    if (IS_FROM_SPACE(aid, maxAid)) {
			      /* this is a from-space chunk */
			        if (IS_BIGCHUNK_AID(aid)) {
				    bigchunk_desc_t	*dp;
				    dp = MajorGC_ForwardBigChunk (
					heap, maxCollectedGen, w, aid);
				    targetGen = dp->gen;
			        }
			        else {
				    *p =
				    w = MajorGC_ForwardChunk(heap, maxAid, w, aid);
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
		    else if (i == maxCollectedGen)
			cm->map[card] = CARD_CLEAN;
		});
#elif (!defined(BIT_CARDS))
		FOR_DIRTY_CARD (cm, maxCollectedGen, card, {
		    lib7_val_t	*p = (cm->baseAddr + (card*CARD_SZW));
		    lib7_val_t	*q = p + CARD_SZW;
		    int		mark = i+1;
COUNT_CARD1(i);
		    if (q > maxSweep)
		      /* don't sweep above the allocation high-water mark */
			q = maxSweep;
		    for (;  p < q;  p++) {
			lib7_val_t	w = *p;
			if (isBOXED(w)) {
			    aid_t	aid = ADDR_TO_PAGEID(bibop, w);
			    int		targetGen;
IFBO_COUNT1(aid);
			    if (IS_FROM_SPACE(aid, maxAid)) {
			      /* this is a from-space chunk */
COUNT_CARD2(i);
			        if (IS_BIGCHUNK_AID(aid)) {
				    bigchunk_desc_t	*dp;
				    dp = MajorGC_ForwardBigChunk (
					heap, maxCollectedGen, w, aid);
				    targetGen = dp->gen;
			        }
			        else {
				    *p =
				    w = MajorGC_ForwardChunk(heap, maxAid, w, aid);
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
		    else if (i == maxCollectedGen)
			cm->map[card] = CARD_CLEAN;
		});
#else
		FOR_DIRTY_CARD (cm, card, {
		    lib7_val_t	*p = (cm->baseAddr + (card*CARD_SZW));
		    lib7_val_t	*q = p + CARD_SZW;
COUNT_CARD(i);
		    if (q > maxSweep)
		      /* don't sweep above the allocation high-water mark */
			q = maxSweep;
		    for (;  p < q;  p++) {
			MajorGC_CheckWord (heap, bibop, maxAid, p);
		    }
		});
#endif
	    }
	}
    } /* end of for */

#ifdef COUNT_CARDS
/*CARD*/SayDebug ("\n[%d] SWEEP: ", maxCollectedGen);
/*CARD*/for(i = maxCollectedGen;  i < heap->numGens;  i++) {
/*CARD*/  card_map_t  *cm = heap->gen[i]->dirty;
/*CARD*/  if (i > maxCollectedGen) SayDebug (", ");
#ifndef BIT_CARDS
/*CARD*/  SayDebug ("[%d] %d/%d/%d", i+1, cardCnt1[i], cardCnt2[i],
/*CARD*/	(cm != NULL) ? cm->numCards : 0);
#else
/*CARD*/  SayDebug ("[%d] %d/%d", i+1, cardCnt[i],
/*CARD*/	(cm != NULL) ? cm->numCards : 0);
#endif
/*CARD*/}
/*CARD*/SayDebug ("\n");
#endif

} /* end of MajorGC_ScanRoots */


/* MajorGC_SweepToSpace:
 * Sweep the to-space arenas.  Because there are few references forward in time, we
 * try to completely scavenge a younger generation before moving on to the
 * next oldest.
 */
static void MajorGC_SweepToSpace (heap_t *heap, int maxCollectedGen, int maxSweptGen)
{
    int		i;
    bool_t	swept;
    bibop_t	bibop = BIBOP;
    aid_t	maxAid = MAKE_MAX_AID(maxCollectedGen);

#define SweepToSpArena(gen, index)	{					\
	arena_t	    *__ap = (gen)->arena[(index)];				\
	if (isACTIVE(__ap)) {							\
	    lib7_val_t    *__p, *__q;						\
	    __p = __ap->sweep_nextw;						\
	    if (__p < __ap->nextw) {						\
		swept = TRUE;							\
		do {								\
		    for (__q = __ap->nextw;  __p < __q;  __p++) {		\
			MajorGC_CheckWord(heap, bibop, maxAid, __p);		\
		    }								\
		} while (__q != __ap->nextw);					\
		__ap->sweep_nextw = __q;					\
	    }									\
	}									\
    } /* SweepToSpArena */

    do {
	swept = FALSE;
	for (i = 0;  i < maxSweptGen;  i++) {
	    gen_t	*gen = heap->gen[i];

	  /* Sweep the record and pair arenas */
	    SweepToSpArena(gen, RECORD_INDEX);
	    SweepToSpArena(gen, PAIR_INDEX);

	  /* Sweep the rw_vector arena */
	    {
		arena_t		*ap = gen->arena[ARRAY_INDEX];
		if (isACTIVE(ap)
		&& MajorGC_SweepToSpArrays (heap, maxCollectedGen, ap, gen->dirty))
		    swept = TRUE;
	    }
	}
    } while (swept);

}/* end of SweepToSpace */


/* MajorGC_SweepToSpArrays:
 *
 * Sweep the to-space of the rw_vector arena, returning true if any chunks
 * are actually swept.
 */
static bool_t MajorGC_SweepToSpArrays (
	heap_t *heap, int maxGen, arena_t *tosp, card_map_t *cm)
{
    lib7_val_t	w, *p, *stop;
    int		thisGen;
    Word_t	cardMask = ~(CARD_SZB - 1);
    aid_t	*bibop = BIBOP;
    aid_t	maxAid = MAKE_MAX_AID(maxGen);
#ifndef BIT_CARDS
    lib7_val_t	*cardStart;
    int		cardMark;
#endif

  /* Sweep a single card at a time, looking for references that need to
   * be remembered.
   */
    thisGen = EXTRACT_GEN(tosp->id);
    p = tosp->sweep_nextw;
    if (p == tosp->nextw)
	return FALSE;
    while (p < tosp->nextw) {
	stop = (lib7_val_t *)(((Addr_t)p + CARD_SZB) & cardMask);
	if (stop > tosp->nextw)
	    stop = tosp->nextw;
      /* Sweep the next page until we see a reference to a younger generation */
#ifndef BIT_CARDS
	cardStart = p;
	cardMark = CARD(cm, cardStart);
#endif
	while (p < stop) {
	    if (isBOXED(w = *p)) {
		aid_t		arena_id = ADDR_TO_PAGEID(bibop, w);
		int		targetGen;

IFBO_COUNT1(arena_id);
		if (IS_FROM_SPACE(arena_id, maxAid)) {
		  /* this is a from-space chunk */
		    if (IS_BIGCHUNK_AID(arena_id)) {
			bigchunk_desc_t	*dp;
			dp = MajorGC_ForwardBigChunk (heap, maxGen, w, arena_id);
			targetGen = dp->gen;
		    }
		    else {
			*p = w = MajorGC_ForwardChunk(heap, maxAid, w, arena_id);
#ifdef TOSPACE_ID
			{ aid_t aid = ADDR_TO_PAGEID(bibop, w);
			  if (IS_TOSPACE_AID(aid))
			    targetGen = TOSPACE_GEN(aid);
			  else
			    targetGen = EXTRACT_GEN(aid);
			}
#else
			targetGen = EXTRACT_GEN(ADDR_TO_PAGEID(bibop, w));
#endif
		    }
#ifndef BIT_CARDS
		    if (targetGen < cardMark)
			cardMark = targetGen;
#else
		    if (targetGen < thisGen) {
		      /* the forwarded chunk is in a younger generation */
			MARK_CARD(cm, p);
		      /* finish the card up quickly */
			for (p++; p < stop;  p++) {
			    MajorGC_CheckWord(heap, bibop, maxAid, p);
			}
			break;
		    }
#endif
		}
#ifdef TOSPACE_ID
		else if (IS_TOSPACE_AID(arena_id)) {
		    Die ("Sweep Arrays: TOSPACE reference: %#x (%#x) --> %#x\n",
			p, ADDR_TO_PAGEID(bibop, p), w);
		}
#endif
	    }
	    p++;
	} /* end of while */
#ifndef BIT_CARDS
	if (cardMark < thisGen)
	    MARK_CARD(cm, cardStart, cardMark);
#endif
    } /* end of while */
    tosp->sweep_nextw = p;

    return TRUE;

} /* end of MajorGC_SweepToSpArrays */


/* MajorGC_ForwardChunk:
 *
 * Forward an chunk.
 */
static lib7_val_t MajorGC_ForwardChunk (heap_t *heap, aid_t maxAid, lib7_val_t v, aid_t id)
{
    lib7_val_t	*chunk = PTR_LIB7toC(lib7_val_t, v);
    lib7_val_t	*new_chunk;
    lib7_val_t	desc;
    Word_t	len;
    arena_t	*arena;

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
	  case DTAG_record:
	    len = GET_LEN(desc);
	    break;
	  default:
	    Die ("bad record tag %d, chunk = %#x, desc = %#x",
		GET_TAG(desc), chunk, desc);
	} /* end of switch */
	arena = heap->gen[EXTRACT_GEN(id)-1]->arena[RECORD_INDEX];
	if (isOLDER(arena, chunk))
	    arena = arena->nextGen;
      } break;

      case CHUNKC_pair: {
	lib7_val_t	w;

	w = chunk[0];
	if (isDESC(w))
	    return PTR_CtoLib7(FOLLOW_FWDPAIR(w, chunk));
	else {
	  /* forward the pair */
	    arena = heap->gen[EXTRACT_GEN(id)-1]->arena[PAIR_INDEX];
	    if (isOLDER(arena, chunk))
		arena = arena->nextGen;
	    new_chunk = arena->nextw;
	    arena->nextw += PAIR_SZW;
	    new_chunk[0] = w;
	    new_chunk[1] = chunk[1];
	  /* setup the forward pointer in the old pair */
	    chunk[0] =  MAKE_PAIR_FP(new_chunk);
	    return PTR_CtoLib7(new_chunk);
	}
      } break;

      case CHUNKC_string: {
	arena = heap->gen[EXTRACT_GEN(id)-1]->arena[STRING_INDEX];
	if (isOLDER(arena, chunk))
	    arena = arena->nextGen;
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
	} /* end of switch */
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
	    return MajorGC_FwdSpecial (heap, maxAid, chunk, id, desc);
	  default:
	    Die ("bad rw_vector tag %d, chunk = %#x, desc = %#x",
		GET_TAG(desc), chunk, desc);
	} /* end of switch */
	arena = heap->gen[EXTRACT_GEN(id)-1]->arena[ARRAY_INDEX];
	if (isOLDER(arena, chunk))
	    arena = arena->nextGen;
      } break;

      case CHUNKC_bigchunk:
	MajorGC_ForwardBigChunk (heap, EXTRACT_GEN(maxAid), v, id);
	return v;

      default:
	Die("unknown chunk ilk %d @ %#x", EXTRACT_CHUNKC(id), chunk);
    } /* end of switch */

  /* Allocate and initialize a to-space copy of the chunk */
    new_chunk = arena->nextw;
    arena->nextw += (len + 1);
    *new_chunk++ = desc;
    ASSERT(arena->nextw <= arena->tospTop);
    COPYLOOP(chunk, new_chunk, len);

  /* set up the forward pointer, and return the new chunk. */
    chunk[-1] = DESC_forwarded;
    chunk[0] = (lib7_val_t)(Addr_t)new_chunk;

    return PTR_CtoLib7(new_chunk);

} /* end of MajorGC_ForwardChunk */


/* MajorGC_ForwardBigChunk:
 *
 * Forward a big-chunk chunk, where id is the BIBOP entry for chunk.
 * Return the descriptor for chunk.
 */
static bigchunk_desc_t *MajorGC_ForwardBigChunk (
	heap_t *heap, int maxGen, lib7_val_t chunk, aid_t id)
{
    int		    i;
    bigchunk_region_t *region;
    bigchunk_desc_t   *dp;

BO2_COUNT;
    for (i = BIBOP_ADDR_TO_INDEX(chunk);  !BO_IS_HDR(id);  id = BIBOP[--i])
	continue;
    region = (bigchunk_region_t *)BIBOP_INDEX_TO_ADDR(i);
    dp = ADDR_TO_BODESC(region, chunk);
    if ((dp->gen <= maxGen) && BO_IS_FROM_SPACE(dp)) {
BO3_COUNT;
      /* forward the big-chunk; note that chunks in the oldest generation
       * will always be YOUNG, thus will never be promoted.
       */
	if (dp->state == BO_YOUNG)
	    dp->state = BO_FORWARD;
	else
	    dp->state = BO_PROMOTE;
    }

    return dp;

} /* end of MajorGC_ForwardBigChunk */


/* MajorGC_FwdSpecial:
 *
 * Forward a special chunk (suspension, weak pointer, ...).
 */
static lib7_val_t MajorGC_FwdSpecial (
    heap_t	*heap,
    aid_t	maxAid,
    lib7_val_t	*chunk,
    aid_t	id,
    lib7_val_t	desc
)
{
    gen_t	*gen = heap->gen[EXTRACT_GEN(id)-1];
    arena_t	*arena = gen->arena[ARRAY_INDEX];
    lib7_val_t	*new_chunk;

    if (isOLDER(arena, chunk))
	arena = arena->nextGen;

  /* allocate the new chunk */
    new_chunk = arena->nextw;
    arena->nextw += SPECIAL_SZW;  /* all specials are two words */

    switch (GET_LEN(desc)) {
      case SPECIAL_evaled_susp:
      case SPECIAL_unevaled_susp:
      case SPECIAL_null_weak:
	*new_chunk++ = desc;
	*new_chunk = *chunk;
	break;
      case SPECIAL_weak: {
	    lib7_val_t	v = *chunk;
#ifdef DEBUG_WEAK_PTRS
SayDebug ("MajorGC: weak [%#x ==> %#x] --> %#x", chunk, new_chunk+1, v);
#endif
	    if (! isBOXED(v)) {
#ifdef DEBUG_WEAK_PTRS
SayDebug (" unboxed\n");
#endif
	      /* weak references to unboxed chunks are never nullified */
		*new_chunk++ = DESC_weak;
		*new_chunk = v;
	    }
	    else {
		aid_t		aid = ADDR_TO_PAGEID(BIBOP, v);
		lib7_val_t	*vp = PTR_LIB7toC(lib7_val_t, v);
		lib7_val_t	desc;

		if (IS_FROM_SPACE(aid, maxAid)) {
		    switch (EXTRACT_CHUNKC(aid)) {
		      case CHUNKC_record:
		      case CHUNKC_string:
		      case CHUNKC_array:
			desc = vp[-1];
			if (desc == DESC_forwarded) {
			  /* Reference to an chunk that has already been forwarded.
			   * NOTE: we have to put the pointer to the non-forwarded
			   * copy of the chunk (i.e, v) into the to-space copy
			   * of the weak pointer, since the GC has the invariant
			   * it never sees to-space pointers during sweeping.
			   */
#ifdef DEBUG_WEAK_PTRS
SayDebug (" already forwarded to %#x\n", FOLLOW_FWDCHUNK(vp));
#endif
			    *new_chunk++ = DESC_weak;
			    *new_chunk = v;
			}
			else {
			  /* the forwarded version of weak chunks are threaded
			   * via their descriptor fields.  We mark the chunk
			   * reference field to make it look like an unboxed value,
			   * so that the to-space sweeper does not follow the weak
			   * reference.
			   */
#ifdef DEBUG_WEAK_PTRS
SayDebug (" forward (start = %#x)\n", vp);
#endif
			    *new_chunk = MARK_PTR(PTR_CtoLib7(gen->heap->weakList));
			    gen->heap->weakList = new_chunk++;
			    *new_chunk = MARK_PTR(vp);
			}
			break;
		      case CHUNKC_pair:
			if (isDESC(desc = vp[0])) {
			  /* Reference to a pair that has already been forwarded.
			   * NOTE: we have to put the pointer to the non-forwarded
			   * copy of the pair (i.e, v) into the to-space copy
			   * of the weak pointer, since the GC has the invariant
			   * it never sees to-space pointers during sweeping.
			   */
#ifdef DEBUG_WEAK_PTRS
SayDebug (" (pair) already forwarded to %#x\n", FOLLOW_FWDPAIR(desc, vp));
#endif
			    *new_chunk++ = DESC_weak;
			    *new_chunk = v;
			}
			else {
			    *new_chunk = MARK_PTR(PTR_CtoLib7(gen->heap->weakList));
			    gen->heap->weakList = new_chunk++;
			    *new_chunk = MARK_PTR(vp);
			}
			break;
		      case CHUNKC_bigchunk:
			Die ("weak big chunk");
			break;
		    }
		}
		else {
		  /* reference to an older chunk */
#ifdef DEBUG_WEAK_PTRS
SayDebug (" old chunk\n");
#endif
		    *new_chunk++ = DESC_weak;
		    *new_chunk = v;
		}
	    }
	} break;
      default:
	Die ("strange/unexpected special chunk @ %#x; desc = %#x\n", chunk, desc);
    } /* end of switch */

    chunk[-1] = DESC_forwarded;
    chunk[0] = (lib7_val_t)(Addr_t)new_chunk;

    return PTR_CtoLib7(new_chunk);

} /* end of MajorGC_FwdSpecial */


/* TrimHeap:
 *
 * After a major collection, trim any arenas that are over their maximum
 * size in allocated space, but under their maximum size in used space.
 */
static void TrimHeap (heap_t *heap, int maxCollectedGen)
{
    int			i, j;
    gen_t		*gen;
    arena_t		*ap;
    Word_t		minSzB, newSzB;

    for (i = 0;  i < maxCollectedGen;  i++) {
	gen = heap->gen[i];
	for (j = 0;  j < NUM_ARENAS;  j++) {
	    ap = gen->arena[j];
	    if (isACTIVE(ap) && (ap->tospSizeB > ap->maxSizeB)) {
		minSzB = (i == 0)
		    ? heap->allocSzB
		    : heap->gen[i-1]->arena[j]->tospSizeB;
		minSzB += (USED_SPACE(ap) + ap->reqSizeB);
		if (minSzB < ap->maxSizeB)
		    newSzB = ap->maxSizeB;
		else {
		    newSzB = RND_HEAP_CHUNK_SZB(minSzB);
		  /* the calculation of minSz here may return something bigger
		   * that what flip.c computed!
		   */
		    if (newSzB > ap->tospSizeB)
			newSzB = ap->tospSizeB;
		}
		ap->tospSizeB = newSzB;
		ap->tospTop = (lib7_val_t *)((Addr_t)ap->tospBase + ap->tospSizeB);
	    }
	}
    }

} /* end of TrimHeap */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
