/* init-gc.c
 *
 * The GC initialization code.
 *
 */

/*
###         "Weave a circle round him thrice,
###            And close your eyes with holy dread,
###          For he on honey-dew hath fed,
###            And drunk the milk of Paradise."
###
###                          -- Coleridge
 */

#ifdef PAUSE_STATS		/* GC pause statistics are UNIX dependent */
#  include "runtime-unixdep.h"
#endif

#include "../config.h"

#include <stdarg.h>
#include <string.h>
#include "runtime-base.h"
#include "runtime-options.h"
#include "runtime-limits.h"
#include "memory.h"
#include "runtime-state.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cntr.h"
#include "heap.h"
#include "heap-monitor.h"
#include "runtime-globals.h"
#include "runtime-timer.h"
#include "gc-stats.h"
#include "runtime-mp.h"

static int		DfltRatios[MAX_NUM_GENS] = {
	DEFAULT_RATIO1,	DEFAULT_RATIO2,	DEFAULT_RATIO,	DEFAULT_RATIO,
	DEFAULT_RATIO,	DEFAULT_RATIO,	DEFAULT_RATIO
    };

#ifdef TWO_LEVEL_MAP
#  error two level map not supported
#else
aid_t		*BIBOP;
#endif

#ifdef COLLECT_STATS			/* Should this go into gc-stats.c ???		*/
bool_t		StatsOn = TRUE;		/* If TRUE, then generate stats.		*/
int		StatsFD = -1;		/* The file descriptor to write the data to.	*/
stat_rec_t	StatsBuf[STATS_BUF_SZ];	/* Buffer of data.				*/
int		NStatsRecs;		/* The number of records in the buffer.		*/
#endif


/* ParseHeapParams:
 *
 * Parse any heap parameters from the command-line arguments.
 */
heap_params_t *ParseHeapParams (char **argv)
{
    char	    option[MAX_OPT_LEN], *option_arg;
    bool_t	    seen_error = FALSE;
    char	    *arg;
    heap_params_t   *params;

    if ((params = NEW_CHUNK(heap_params_t)) == NULL) {
	Die("unable to allocate heap_params");
    }

    /* We use 0 or "-1" to signify that
     * the default value should be used:
     */
    params->allocSz = 0;
    params->numGens = -1;
    params->cacheGen = -1;

#define MATCH(opt)	(strcmp(opt, option) == 0)
#define CHECK(opt)	{						\
	if (option_arg[0] == '\0') {					\
	    seen_error = TRUE;						\
	    Error("missing argument for \"%s\" option\n", opt);		\
	    continue;							\
	}								\
    } /* CHECK */

    while ((arg = *argv++) != NULL) {

	if (is_runtime_option(arg, option, &option_arg)) {

	    if (MATCH("gc-gen0-bufsize")) { /* set garbage collector generation-zero buffer size */

		CHECK("gc-gen0-bufsize");
		if ((params->allocSz = get_size_option(ONE_K, option_arg)) < 0) {
		    seen_error = TRUE;
		    Error ("bad argument for \"--runtime-gc-gen0-bufsize\" option\n");
		}

	    } else if (MATCH("gc-generations")) {

		CHECK("gc-generations");
		params->numGens = atoi(option_arg);
		if (params->numGens < 1)
		    params->numGens = 1;
		else if (params->numGens > MAX_NGENS)
		    params->numGens = MAX_NGENS;

	    } else if (MATCH("vmcache")) {

		CHECK("vmcache");
		params->cacheGen = atoi(option_arg);
		if (params->cacheGen < 0)
		    params->cacheGen = 0;
		else if (params->cacheGen > MAX_NGENS)
		    params->cacheGen = MAX_NGENS;

	    } else if (MATCH("unlimited-heap")) {

		UnlimitedHeap = TRUE;
	    }
	}

	if (seen_error)  return NULL;
    }						/* while */

    return params;
}

void   set_up_heap   (   lib7_state_t*   lib7_state,
                      bool_t           is_boot,
                      heap_params_t*   params
) {
    /***********************************/
    /* Create and initialize the heap. */
    /***********************************/

    int		i;
    int		j;
    int		ratio;
    int		max_size;
    heap_t*	heap;
    gen_t*	gen;

    heap_chunk_t*	baseChunk;
    lib7_val_t*	allocBase;

    if (params->allocSz == 0) params->allocSz = DEFAULT_ALLOC;
    if (params->numGens < 0)  params->numGens = DEFAULT_NGENS;
    if (params->cacheGen < 0) params->cacheGen = DEFAULT_CACHE_GEN;

    /* First we initialize the underlying memory system:
    */
    MEM_InitMemory ();

    /* Allocate the base memory chunk -- holds
     * the BIBOP and allocation space:
     */
    {   long	bibopSz;

#ifdef TWO_LEVEL_MAP
#  error two level map not supported
#else
	bibopSz = BIBOP_SZ * sizeof(aid_t);
#endif
	baseChunk = allocate_heap_chunk (MAX_NUM_PROCS*params->allocSz + bibopSz);
	if (baseChunk == NULL)
	    Die ("unable to allocate memory chunk for BIBOP");
	BIBOP = (bibop_t)HEAP_CHUNK_BASE(baseChunk);
	allocBase = (lib7_val_t *)(((Addr_t)BIBOP) + bibopSz);
    }

    /* Initialize the BIBOP:
    */
#ifdef TWO_LEVEL_MAP
#  error two level map not supported
#else
    for (i = 0;  i < BIBOP_SZ;  i++)
	BIBOP[i] = AID_UNMAPPED;
#endif

    /* Initialize heap descriptor:
    */
    heap = NEW_CHUNK(heap_t);
    memset ((char *)heap, 0, sizeof(heap_t));
    for (i = 0;  i < MAX_NUM_GENS;  i++) {
	ratio = DfltRatios[i];
	if (i == 0)
	    max_size = MAX_SZ1(params->allocSz * MAX_NUM_PROCS);
	else {
	    max_size = (5*max_size)/2;
	    if (max_size > 64*ONE_MEG) max_size = 64*ONE_MEG;
	}
	gen		=
	heap->gen[i]	= NEW_CHUNK(gen_t);
	gen->heap	= heap;
	gen->genNum	= i+1;
	gen->numGCs	= 0;
	gen->lastPrevGC	= 0;
	gen->ratio	= ratio;
	gen->toChunk	= NULL;
	gen->fromChunk	= NULL;
	gen->cacheChunk	= NULL;
	gen->dirty	= NULL;
	for (j = 0;  j < NUM_ARENAS;  j++) {
	    gen->arena[j] = NEW_CHUNK(arena_t);
	    gen->arena[j]->tospSizeB = 0;
	    gen->arena[j]->reqSizeB = 0;
	    gen->arena[j]->maxSizeB = max_size;
	    gen->arena[j]->id = MAKE_AID(i+1, j+1, 0);
	}
	for (j = 0;  j < NUM_BIGCHUNK_KINDS;  j++)
	    gen->bigChunks[j] = NULL;
    }
    for (i = 0;  i < params->numGens;  i++) {
	int	k = (i == params->numGens-1) ? i : i+1;
	for (j = 0;  j < NUM_ARENAS;  j++) {

	    heap->gen[i]->arena[j]->nextGen
                =
                heap->gen[k]->arena[j];
	}
    }
    heap->numGens		= params->numGens;
    heap->cacheGen		= params->cacheGen;
    heap->numMinorGCs		= 0;
    heap->numBORegions		= 0;
    heap->bigRegions		= NULL;
    heap->freeBigChunks		= NEW_CHUNK(bigchunk_desc_t);
    heap->freeBigChunks->chunk	= (Addr_t)0;
    heap->freeBigChunks->sizeB	= 0;
    heap->freeBigChunks->state	= BO_FREE;
    heap->freeBigChunks->prev	= heap->freeBigChunks;
    heap->freeBigChunks->next	= heap->freeBigChunks;
    heap->weakList		= NULL;

    /* Initialize new space:
    */
    heap->baseChunk = baseChunk;
    heap->allocBase = allocBase;
    heap->allocSzB = MAX_NUM_PROCS*params->allocSz;
    MarkRegion (BIBOP, (lib7_val_t *)BIBOP, HEAP_CHUNK_SZB(heap->baseChunk), AID_NEW);
#ifdef VERBOSE
    SayDebug ("NewSpace = [%#x, %#x:%#x), %d bytes\n",
	heap->allocBase, HEAP_LIMIT(heap),
	(Word_t)(heap->allocBase)+params->allocSz, params->allocSz);
#endif

#ifdef GC_STATS
    ClearGCStats (heap);
#endif
#if defined(COLLECT_STATS)
    if (StatsFD > 0) {
	stat_hdr_t	header;
	CNTR_ZERO(&(heap->numAlloc));
	header.mask = STATMASK_ALLOC|STATMASK_NGENS|STATMASK_START|STATMASK_STOP;
	header.isNewRuntime = 1;
	header.allocSzB = params->allocSz;
	header.numGens = params->numGens;
	gettimeofday (&(header.startTime), NULL);
	write (StatsFD, (char *)&header, sizeof(stat_hdr_t));
    }
#endif

    if (is_boot) {
      /* Create the first generation's to-space. */
	for (i = 0;  i < NUM_ARENAS;  i++)
	    heap->gen[0]->arena[i]->tospSizeB = RND_HEAP_CHUNK_SZB(2 * heap->allocSzB);
	if (NewGeneration(heap->gen[0]) == FAILURE)
	    Die ("unable to allocate initial first generation space\n");
	for (i = 0;  i < NUM_ARENAS;  i++)
	    heap->gen[0]->arena[i]->oldTop = heap->gen[0]->arena[i]->tospBase;
    }

    /* Initialize the GC related parts of the Lib7 state:
    */
    lib7_state->lib7_heap	= heap;
    lib7_state->lib7_heap_cursor	= (lib7_val_t *)(lib7_state->lib7_allocArena);
#ifdef SOFT_POLL
    ResetPollLimit (lib7_state);
#else
    lib7_state->lib7_heap_limit	= HEAP_LIMIT(heap);
#endif

} /* end of set_up_heap */


#ifdef GC_STATS
/* ClearGCStats:
 */
void ClearGCStats (heap_t *heap)
{
    int		i, j;

    CNTR_ZERO(&(heap->numAlloc));
    for (i = 0;  i < MAX_NUM_GENS;  i++) {
	for (j = 0;  j < NUM_ARENAS;  j++) {
	    CNTR_ZERO(&(heap->numCopied[i][j]));
	}
    }

} /* end of ClearStats */
#endif

/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

