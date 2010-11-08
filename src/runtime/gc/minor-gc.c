/* minor-gc.c
 *
 * This is the code for doing minor collections (i.e., collecting the
 * allocation arena).
 */

/*
###            "It goes against the grain of modern
###             education to teach children to program.
###
###             What fun is there in making plans,
###             acquiring discipline in organizing thoughts,
###             devoting attention to detail, and
###             learning to be self-critical?"
###
###                               -- Alan Perlis
 */

#include "../config.h"

#include "runtime-base.h"
#include "runtime-limits.h"
#include "runtime-state.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "card-map.h"
#include "heap.h"
#include "tags.h"
#include "copy-loop.h"
#ifdef MP_SUPPORT
#include "vproc-state.h"
#endif

#ifdef GC_STATS
extern long	numUpdates;
extern long	numBytesAlloc;
extern long	numBytesCopied;
#endif

/** store list operations */
#define STL_nil		LIB7_void
#define STL_hd(p)	REC_SELPTR(lib7_val_t, p, 0)
#define STL_tl(p)	REC_SEL(p, 1)

/* local routines */
static void MinorGC_ScanStoreList (heap_t *heap, lib7_val_t stl);
static void MinorGC_SweepToSpace (gen_t *gen1);
static lib7_val_t MinorGC_ForwardChunk (gen_t *gen1, lib7_val_t v);
static lib7_val_t MinorGC_FwdSpecial (gen_t *gen1, lib7_val_t *chunk, lib7_val_t desc);

#ifdef VERBOSE
extern char	*ArenaName[];
#endif

/* Check a word for a allocation space reference */
#ifndef NO_GC_INLINE
#define MinorGC_CheckWord(bibop, g1, p)	{					\
	lib7_val_t	__w = *(p);						\
	if (isBOXED(__w) && (ADDR_TO_PAGEID(bibop, __w) == AID_NEW))		\
	    *(p) = MinorGC_ForwardChunk(g1, __w);					\
    }
#else
static void MinorGC_CheckWord (bibop_t bibop, gen_t *g1, lib7_val_t *p)
{
    lib7_val_t	w = *(p);
    if (isBOXED(w)) {
	aid_t	aid = ADDR_TO_PAGEID(bibop, w);
	if (aid == AID_NEW)
	    *(p) = MinorGC_ForwardChunk(g1, w);
    }
}
#endif


/* MinorGC:
 *
 * Do a collection of the allocation space.
 */
void MinorGC (lib7_state_t *lib7_state, lib7_val_t **roots)
{
    heap_t	*heap = lib7_state->lib7_heap;
    gen_t	*gen1 = heap->gen[0];
#ifdef GC_STATS
    long	nbytesAlloc, nbytesCopied, nUpdates=numUpdates;
    Addr_t	gen1Top[NUM_ARENAS];
    int		i;
    {
	nbytesAlloc = (Addr_t)(lib7_state->lib7_heap_cursor) - (Addr_t)(heap->allocBase);
	CNTR_INCR(&(heap->numAlloc), nbytesAlloc);
	for (i = 0;  i < NUM_ARENAS;  i++)
	    gen1Top[i] = (Addr_t)(gen1->arena[i]->nextw);
    }
#elif defined(VM_STATS)
    {
	Addr_t	    nbytesAlloc;
	nbytesAlloc = ((Addr_t)(lib7_state->lib7_heap_cursor) - (Addr_t)(heap->allocBase));
	CNTR_INCR(&(heap->numAlloc), nbytesAlloc);
    }
#endif

#ifdef VERBOSE
{
  int i;
  SayDebug ("Generation 1 before MinorGC:\n");
  for (i = 0;  i < NUM_ARENAS;  i++) {
    SayDebug ("  %s: base = %#x, oldTop = %#x, nextw = %#x\n",
      ArenaName[i+1], gen1->arena[i]->tospBase,
      gen1->arena[i]->oldTop, gen1->arena[i]->nextw);
  }
}
#endif

  /* scan the standard roots */
    {
	lib7_val_t	*rp;
	bibop_t		bibop = BIBOP;

	while ((rp = *roots++) != NULL) {
	    MinorGC_CheckWord(bibop, gen1, rp);
	}
    }

  /* Scan the store list */
#ifdef MP_SUPPORT
    {
	lib7_val_t	stl;
	int		i;
	lib7_state_t	*lib7_state;
	vproc_state_t   *vsp;

	for (i = 0; i < MAX_NUM_PROCS; i++) {
	    vsp = VProc[i];
	    lib7_state = vsp->vp_state;
	    if ((vsp->vp_mpState == MP_PROC_RUNNING)
	    && ((stl = lib7_state->lib7_store_log) != STL_nil)) {
		MinorGC_ScanStoreList (heap, stl);
		lib7_state->lib7_store_log = STL_nil;
	    }
	}
    }
#else
    {
	lib7_val_t	stl = lib7_state->lib7_store_log;
	if (stl != STL_nil) {
	    MinorGC_ScanStoreList (heap, stl);
	    lib7_state->lib7_store_log = STL_nil;
	}
    }
#endif

  /* Sweep the first generation to-space */
    MinorGC_SweepToSpace (gen1);
    heap->numMinorGCs++;

  /* Handle weak pointers */
    if (heap->weakList != NULL)
	ScanWeakPtrs (heap);

#ifdef VERBOSE
{
  int i;
  SayDebug ("Generation 1 after MinorGC:\n");
  for (i = 0;  i < NUM_ARENAS;  i++) {
    SayDebug ("  %s: base = %#x, oldTop = %#x, nextw = %#x\n",
      ArenaName[i+1], gen1->arena[i]->tospBase,
      gen1->arena[i]->oldTop, gen1->arena[i]->nextw);
  }
}
#endif

#ifdef GC_STATS
    {
	int	nbytes;

	nbytesCopied = 0;
	for (i = 0;  i < NUM_ARENAS;  i++) {
	    nbytes = ((Word_t)(gen1->arena[i]->nextw) - gen1Top[i]);
	    nbytesCopied += nbytes;
	    CNTR_INCR(&(heap->numCopied[0][i]), nbytes);
	}

numBytesAlloc += nbytesAlloc;
numBytesCopied += nbytesCopied;
#ifdef XXX
SayDebug ("Minor GC: %d/%d (%5.2f%%) bytes copied; %d updates\n",
nbytesCopied, nbytesAlloc,
(nbytesAlloc ? (double)(100*nbytesCopied)/(double)nbytesAlloc : 0.0),
numUpdates-nUpdates);
#endif
    }
#endif

#ifdef CHECK_HEAP
    CheckHeap(heap, 1);
#endif

} /* end of MinorGC. */


/* MinorGC_ScanStoreList:
 *
 * Scan the store list.  The store list pointer (stl) is guaranteed to
 * be non-null.
 */
static void MinorGC_ScanStoreList (heap_t *heap, lib7_val_t stl)
{
    lib7_val_t	*addr, w;
    gen_t	*gen1 = heap->gen[0];
    bibop_t	bibop = BIBOP;
#ifdef GC_STATS
    int		nUpdates = 0;
#endif

  /* Scan the store list */
    do {
#ifdef GC_STATS
	nUpdates++;
#endif
	addr = STL_hd(stl);
	stl = STL_tl(stl);
	w = *addr;
	if (isBOXED(w)) {
	    aid_t	srcId = ADDR_TO_PAGEID(bibop, addr);
	  /* We can ignore updates to chunks in new-space, and to references
	   * in the runtime system references (ie, UNMAPPED)
	   */
	    if ((srcId != AID_NEW) && (! isUNMAPPED(srcId))) {
	      /* srcGen is the generation of the updated cell; dstGen is the
	       * generation of the chunk that the cell points to.
	       */
		int	srcGen = EXTRACT_GEN(srcId);
		aid_t	dstId = ADDR_TO_PAGEID(bibop, w);
		int	dstGen = EXTRACT_GEN(dstId);

		if (IS_BIGCHUNK_AID(dstId)) {
		    int		    i;
		    bigchunk_region_t *region;
		    bigchunk_desc_t   *dp;
		    if (dstGen >= srcGen)
			continue;
		    for (i = BIBOP_ADDR_TO_INDEX(w);  !BO_IS_HDR(dstId);  dstId = BIBOP[--i])
			continue;
		    region = (bigchunk_region_t *)BIBOP_INDEX_TO_ADDR(i);
		    dp = ADDR_TO_BODESC(region, w);
		    dstGen = dp->gen;
		}
		else {
		    if (dstGen == ALLOC_GEN) {
		      /* The refered to chunk is in allocation space, and will be
		       * forwarded to the first generation.
		       */
			dstGen = 1;
			*addr = MinorGC_ForwardChunk(gen1, w);
		    }
		}
		if (srcGen > dstGen) {
		  /* mark the card containing "addr" */
#ifndef BIT_CARDS
		    MARK_CARD(heap->gen[srcGen-1]->dirty, addr, dstGen);
#else
		    MARK_CARD(heap->gen[srcGen-1]->dirty, addr);
#endif
		}
	    }
	}
    } while (stl != STL_nil);

#ifdef GC_STATS
    numUpdates += nUpdates;
#endif

} /* end MinorGC_ScanStoreList */


/* MinorGC_SweepToSpace:
 *
 * Sweep the first generation's to-space.  Note, that since there are
 * no younger chunks, we don't have to do anything special for the
 * rw_vector space.
 */
static void MinorGC_SweepToSpace (gen_t *gen1)
{
    bibop_t	bibop = BIBOP;
    bool_t	swept;

#define MinorGC_SweepToSpArena(index)	{				\
	arena_t		*__ap = gen1->arena[(index)];			\
	lib7_val_t	*__p, *__q;					\
	__p = __ap->sweep_nextw;					\
	if (__p < __ap->nextw) {					\
	    swept = TRUE;						\
	    do {							\
		for (__q = __ap->nextw;  __p < __q;  __p++)		\
		    MinorGC_CheckWord(bibop, gen1, __p);		\
	    } while (__q != __ap->nextw);				\
	    __ap->sweep_nextw = __q;					\
	}								\
    } /* MinorGC_SweepToSpArena */

    do {
	swept = FALSE;

        /* Sweep the record, pair and rw_vector arenas */
	MinorGC_SweepToSpArena(RECORD_INDEX);
	MinorGC_SweepToSpArena(PAIR_INDEX);
	MinorGC_SweepToSpArena(ARRAY_INDEX);

    } while (swept);

} /* end of MinorGC_SweepToSpace. */

/* MinorGC_ForwardChunk:
 *
 * Forward an chunk from the allocation space to the first generation.
 */
static lib7_val_t MinorGC_ForwardChunk (gen_t *gen1, lib7_val_t v)
{
    lib7_val_t	*chunk = PTR_LIB7toC(lib7_val_t, v);
    lib7_val_t	*new_chunk, desc;
    Word_t	len;
    arena_t	*arena;

    desc = chunk[-1];
    switch (GET_TAG(desc)) {
      case DTAG_record:
	len = GET_LEN(desc);
#ifdef NO_PAIR_STRIP
	arena = gen1->arena[RECORD_INDEX];
#else
	if (len == 2) {
	    arena = gen1->arena[PAIR_INDEX];
	    new_chunk = arena->nextw;
	    arena->nextw += 2;
	    new_chunk[0] = chunk[0];
	    new_chunk[1] = chunk[1];
	    /* Set up the forward pointer in the old pair */
	    chunk[-1] = DESC_forwarded;
	    chunk[0] = (lib7_val_t)(Addr_t)new_chunk;
	    return PTR_CtoLib7(new_chunk);
	}
	else
	    arena = gen1->arena[RECORD_INDEX];
#endif
	break;
      case DTAG_ro_vec_hdr:
      case DTAG_rw_vec_hdr:
	len = 2;
	arena = gen1->arena[RECORD_INDEX];
	break;
      case DTAG_rw_vec_data:
	len = GET_LEN(desc);
	arena = gen1->arena[ARRAY_INDEX];
	break;
      case DTAG_raw32:
	len = GET_LEN(desc);
	arena = gen1->arena[STRING_INDEX];
	break;
      case DTAG_raw64:
	len = GET_LEN(desc);
	arena = gen1->arena[STRING_INDEX];
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
      case DTAG_special:
	return MinorGC_FwdSpecial (gen1, chunk, desc);
      case DTAG_forward:
	return PTR_CtoLib7(FOLLOW_FWDCHUNK(chunk));
      default:
	Die ("bad chunk tag %d, chunk = %#x, desc = %#x", GET_TAG(desc), chunk, desc);
    } /* end of switch */

    /* Allocate and initialize a to-space copy of the chunk:
     */
    new_chunk = arena->nextw;
    arena->nextw += (len + 1);
    *new_chunk++ = desc;
    ASSERT(arena->nextw <= arena->tospTop);

    COPYLOOP(chunk, new_chunk, len);

    /* Set up the forward pointer, and return the new chunk:
     */
    chunk[-1] = DESC_forwarded;
    chunk[0] = (lib7_val_t)(Addr_t)new_chunk;

    return PTR_CtoLib7(new_chunk);

} /* end of MinorGC_ForwardChunk */


/* MinorGC_FwdSpecial:
 *
 * Forward a special chunk (suspension, weak pointer, ...).
 */
static lib7_val_t MinorGC_FwdSpecial (gen_t *gen1, lib7_val_t *chunk, lib7_val_t desc)
{
    arena_t	*arena = gen1->arena[ARRAY_INDEX];
    lib7_val_t	*new_chunk = arena->nextw;

    arena->nextw += SPECIAL_SZW;  /* all specials are two words */

    switch (GET_LEN(desc)) {
      case SPECIAL_evaled_susp:
      case SPECIAL_unevaled_susp:
	*new_chunk++ = desc;
	*new_chunk = *chunk;
	break;
      case SPECIAL_weak: {
	    lib7_val_t	v = *chunk;
#ifdef DEBUG_WEAK_PTRS
SayDebug ("MinorGC: weak [%#x ==> %#x] --> %#x", chunk, new_chunk+1, v);
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

		if (aid == AID_NEW) {
		    if (vp[-1] == DESC_forwarded) {
		      /* Reference to a chunk that has already been forwarded.
		       * NOTE: we have to put the pointer to the non-forwarded
		       * copy of the chunk (i.e, v) into the to-space copy
		       * of the weak pointer, since the GC has the invariant
		       * it never sees to-space pointers during sweeping.
		       */
#ifdef DEBUG_WEAK_PTRS
SayDebug (" already forwarded to %#x\n", PTR_CtoLib7(FOLLOW_FWDCHUNK(vp)));
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
SayDebug (" forward\n");
#endif
			*new_chunk = MARK_PTR(PTR_CtoLib7(gen1->heap->weakList));
			gen1->heap->weakList = new_chunk++;
			*new_chunk = MARK_PTR(vp);
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
      case SPECIAL_null_weak: /* shouldn't happen in the allocation arena */
      default:
	Die ("strange/unexpected special chunk @ %#x; desc = %#x\n", chunk, desc);
    } /* end of switch */

    chunk[-1] = DESC_forwarded;
    chunk[0] = (lib7_val_t)(Addr_t)new_chunk;

    return PTR_CtoLib7(new_chunk);

} /* end of MinorGC_FwdSpecial */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

