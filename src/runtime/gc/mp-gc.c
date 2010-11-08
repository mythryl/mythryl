/* mp-gc.c
 *
 * Extra routines to support GC in the MP implementation.
 *
 */

#include "../config.h"


#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <stdarg.h>
#include "runtime-base.h"
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
#include "vproc-state.h"

/* MP_SUPPORT */

/* PartitionAllocArena:
 *
 * Divide this allocation arena into smaller disjoint arenas for
 * use by the parallel processors.
 */
void PartitionAllocArena (vproc_state_t *vsps[])
{
    int		indivSz;
    lib7_val_t	*aBase;
    int		i;
    int pollFreq = INT_LIB7toC(DEREF(PollFreq));
    lib7_state_t  *lib7_state, *lib7_state0;

    lib7_state0 = vsps[0]->vp_state;
    indivSz = lib7_state0->lib7_heap->allocSzB / MAX_NUM_PROCS;
    aBase = lib7_state0->lib7_heap->allocBase;
    for (i = 0; i < MAX_NUM_PROCS; i++) {
	lib7_state = vsps[i]->vp_state;
#ifdef MP_DEBUG
SayDebug ("vsps[%d]->vp_state-> (lib7_heap_cursor %x/lib7_heap_limit %x) changed to ",
i, lib7_state->lib7_heap_cursor, lib7_state->lib7_heap_limit);
#endif
	lib7_state->lib7_heap = lib7_state0->lib7_heap;
	lib7_state->lib7_heap_cursor = aBase;
	lib7_state->lib7_real_heap_limit = HEAP_LIMIT_SIZE(aBase, indivSz);

#ifdef MP_GCPOLL
	if (pollFreq > 0) {
#ifdef MP_DEBUG
SayDebug ("(with PollFreq=%d) ", pollFreq);
#endif
	    lib7_state->lib7_heap_limit = aBase + pollFreq*POLL_GRAIN_CPSI;
	    lib7_state->lib7_heap_limit =
		(lib7_state->lib7_heap_limit > lib7_state->lib7_real_heap_limit)
		    ? lib7_state->lib7_real_heap_limit
		    : lib7_state->lib7_heap_limit;

	}
	else {
	    lib7_state->lib7_heap_limit = lib7_state->lib7_real_heap_limit;
	}
#else
	lib7_state->lib7_heap_limit = HEAP_LIMIT_SIZE(aBase,indivSz);
#endif

#ifdef MP_DEBUG
SayDebug ("%x/%x\n",lib7_state->lib7_heap_cursor, lib7_state->lib7_heap_limit);
#endif
	aBase = (lib7_val_t *) (((Addr_t) aBase) + indivSz);
    }

} /* end of PartitionAllocArena */


static volatile int    MP_RdyForGC = 0;	/* the number of processors that are */
					/* ready for the GC. */
static int		    MPCollectorProc;	/* the processor that does the GC */

/* Extra roots provided by collect_garbage_with_extra_roots go here: */
lib7_val_t            *mpExtraRoots[NUM_EXTRA_ROOTS*MAX_NUM_PROCS];
static lib7_val_t        **mpExtraRootsPtr;

/* MP_StartCollect:
 *
 * Waits for all procs to check in and chooses one to do the 
 * collect (MPCollectorProc).  MPCollectorProc returns to the invoking
 * collection function and does the collect while the other procs
 * wait at a barrier. MPCollectorProc will eventually check into this
 * barrier releasing the waiting procs.
 */
int MP_StartCollect (lib7_state_t *lib7_state)
{
    int		nProcs;
    vproc_state_t *vsp = lib7_state->lib7_vproc;

    MP_SetLock(MP_GCLock);
    if (MP_RdyForGC++ == 0) {
        mpExtraRoots[0] = NULL;
        mpExtraRootsPtr = mpExtraRoots;
#ifdef MP_GCPOLL
	ASSIGN(PollEvent, LIB7_true);
#ifdef MP_DEBUG
	SayDebug ("%d: set poll event\n", lib7_state->lib7_mpSelf);
#endif
#endif
      /* we're the first one in, we'll do the collect */
	MPCollectorProc = vsp->vp_mpSelf;
#ifdef MP_DEBUG
	SayDebug ("MPCollectorProc is %d\n",MPCollectorProc);
#endif
    }
    MP_UnsetLock(MP_GCLock);

    {
#ifdef MP_DEBUG
	int n = 0;
#endif
      /* nb: some other proc can be concurrently acquiring new processes */
	while (MP_RdyForGC !=  (nProcs = MP_ActiveProcs())) {
	  /* spin */
#ifdef MP_DEBUG
	    if (n == 10000000) {
		n = 0;
		SayDebug ("%d spinning %d <> %d <alloc=0x%x, limit=0x%x>\n", 
		    lib7_state->lib7_mpSelf, MP_RdyForGC, nProcs, lib7_state->lib7_heap_cursor,
		    lib7_state->lib7_heap_limit);
	    }
	    else
	      n++;
#endif
	}
    }

  /* Here, all of the processors are ready to do GC */

#ifdef MP_GCPOLL
    ASSIGN(PollEvent, LIB7_false);
#ifdef MP_DEBUG
    SayDebug ("%d: cleared poll event\n", lib7_state->lib7_mpSelf);
#endif
#endif
#ifdef MP_DEBUG
    SayDebug ("(%d) all %d/%d procs in\n", lib7_state->lib7_mpSelf, MP_RdyForGC, MP_ActiveProcs());
#endif
    if (MPCollectorProc != vsp->vp_mpSelf) {
#ifdef MP_DEBUG
	SayDebug ("%d entering barrier %d\n",vsp->vp_mpSelf,nProcs);
#endif
	MP_Barrier(MP_GCBarrier, nProcs);
    
#ifdef MP_DEBUG
	SayDebug ("%d left barrier\n", vsp->vp_mpSelf);
#endif
	return 0;
    }

    return nProcs;

} /* end of MP_StartCollect */


/* MP_StartCollectWithRoots:
 *
 * as above, but collects extra roots into mpExtraRoots
 */
int MP_StartCollectWithRoots (lib7_state_t *lib7_state, va_list ap)
{
    int		nProcs;
    lib7_val_t    *p;
    vproc_state_t *vsp = lib7_state->lib7_vproc;

    MP_SetLock(MP_GCLock);
    if (MP_RdyForGC++ == 0) {
        mpExtraRootsPtr = mpExtraRoots;
#ifdef MP_GCPOLL
	ASSIGN(PollEvent, LIB7_true);
#ifdef MP_DEBUG
	SayDebug ("%d: set poll event\n", vsp->vp_mpSelf);
#endif
#endif
      /* we're the first one in, we'll do the collect */
	MPCollectorProc = vsp->vp_mpSelf;
#ifdef MP_DEBUG
	SayDebug ("MPCollectorProc is %d\n",MPCollectorProc);
#endif
    }
    while ((p = va_arg(ap, lib7_val_t *)) != NULL) {
	*mpExtraRootsPtr++ = p;
    }
    *mpExtraRootsPtr = p;  /* NULL */
    MP_UnsetLock(MP_GCLock);

    {
#ifdef MP_DEBUG
	int n = 0;
#endif
      /* nb: some other proc can be concurrently acquiring new processes */
	while (MP_RdyForGC !=  (nProcs = MP_ActiveProcs())) {
	  /* spin */
#ifdef MP_DEBUG
	    if (n == 10000000) {
		n = 0;
		SayDebug ("%d spinning %d <> %d <alloc=0x%x, limit=0x%x>\n", 
		    vsp->vp_mpSelf, MP_RdyForGC, nProcs, lib7_state->lib7_heap_cursor,
		    lib7_state->lib7_heap_limit);
	    }
	    else
	      n++;
#endif
	}
    }

  /* Here, all of the processors are ready to do GC */

#ifdef MP_GCPOLL
    ASSIGN(PollEvent, LIB7_false);
#ifdef MP_DEBUG
    SayDebug ("%d: cleared poll event\n", lib7_state->lib7_mpSelf);
#endif
#endif
#ifdef MP_DEBUG
    SayDebug ("(%d) all %d/%d procs in\n", lib7_state->lib7_vproc->vp_mpSelf, MP_RdyForGC, MP_ActiveProcs());
#endif
    if (MPCollectorProc != vsp->vp_mpSelf) {
#ifdef MP_DEBUG
	SayDebug ("%d entering barrier %d\n", vsp->vp_mpSelf, nProcs);
#endif
	MP_Barrier(MP_GCBarrier, nProcs);
    
#ifdef MP_DEBUG
	SayDebug ("%d left barrier\n", vsp->vp_mpSelf);
#endif
	return 0;
    }

    return nProcs;

} /* end of MP_StartCollectWithRoots */


/* MP_FinishCollect:
 */
void MP_FinishCollect (lib7_state_t *lib7_state, int n)
{
  /* this works, but PartitionAllocArena is overkill */
    PartitionAllocArena(VProc);
    MP_SetLock(MP_GCLock);
#ifdef MP_DEBUG
    SayDebug ("%d entering barrier %d\n", lib7_state->lib7_vproc->vp_mpSelf,n);
#endif
    MP_Barrier(MP_GCBarrier,n);
    MP_RdyForGC = 0;

#ifdef MP_DEBUG
    SayDebug ("%d left barrier\n", lib7_state->lib7_vproc->vp_mpSelf);
#endif
    MP_UnsetLock(MP_GCLock);

} /* end of MP_FinishCollect */



/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

