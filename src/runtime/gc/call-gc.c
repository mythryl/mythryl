/* call-gc.c
 *
 * The main interface between the garbage collector
 * and the rest of the run-time system.
 *
 * These are the routines used to invoke the garbage collector.
 */

#ifdef PAUSE_STATS		/* GC pause statistics are UNIX dependent */
#  include "runtime-unixdep.h"
#endif

#include "../config.h"

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
#include "vproc-state.h"
#include "profile.h"

#ifdef C_CALLS
/* This is a list of pointers into the C heap locations that hold
 * pointers to Lib7 functions. This list is not part of any Lib7 data
 * package(s).  (also see gc/major-gc.c and c-libs/c-calls/c-calls-fns.c)
 */
extern lib7_val_t		CInterfaceRootList;
#endif


void   collect_garbage (lib7_state_t *lib7_state, int level)
{
    /* Do a garbage collection.  A garbage collection always involves
     * collecting the allocation space.  In addition, if level is greater than
     * 0, or if the first generation is full after the minor collection, then
     * a major collection of one or more generations is performed (at least
     * level generations are collected).
     */

    lib7_val_t	*roots[NUM_GC_ROOTS];	/* registers and globals */
    lib7_val_t	**rootsPtr = roots;
    heap_t	*heap;
    int		i;
#ifdef MP_SUPPORT
    int		nProcs;
#endif

    ASSIGN(ProfCurrent, PROF_MINOR_GC);				/* ProfCurrent is #defined in   src/runtime/include/runtime-globals.h   in terms of   _ProfCurrent   from   src/runtime/main/globals.c	*/

#ifdef MP_SUPPORT
#ifdef MP_DEBUG
    SayDebug ("igc %d\n", lib7_state->lib7_mpSelf);
#endif
    if ((nProcs = MP_StartCollect (lib7_state)) == 0) {
      /* a waiting proc */
	ASSIGN(ProfCurrent, PROF_RUNTIME);
	return;
    }
#endif

    START_GC_PAUSE(lib7_state->lib7_heap);

#ifdef C_CALLS
    *rootsPtr++ = &CInterfaceRootList;
#endif

#ifdef MP_SUPPORT
  /* get extra roots from procs that entered through collect_garbage_with_extra_roots */
    for (i = 0;  mpExtraRoots[i] != NULL; i++)
	*rootsPtr++ = mpExtraRoots[i];
#endif

  /* Gather the roots */
    for (i = 0;  i < NumCRoots;  i++)
	*rootsPtr++ = CRoots[i];
#ifdef MP_SUPPORT
    {
	vproc_state_t   *vsp;
	lib7_state_t	*lib7_state;
	int		j;
      
	for (j = 0; j < MAX_NUM_PROCS; j++) {
	    vsp = VProc[j];
	    lib7_state = vsp->vp_state;
#ifdef MP_DEBUG
	SayDebug ("lib7_state[%d] alloc/limit was %x/%x\n",
	    j, lib7_state->lib7_heap_cursor, lib7_state->lib7_heap_limit);
#endif
	    if (vsp->vp_mpState == MP_PROC_RUNNING) {
		*rootsPtr++ = &(lib7_state->lib7_argument);
		*rootsPtr++ = &(lib7_state->lib7_fate);
		*rootsPtr++ = &(lib7_state->lib7_closure);
		*rootsPtr++ = &(lib7_state->lib7_exception_fate);
		*rootsPtr++ = &(lib7_state->lib7_current_thread);
		*rootsPtr++ = &(lib7_state->lib7_calleeSave[0]);
		*rootsPtr++ = &(lib7_state->lib7_calleeSave[1]);
		*rootsPtr++ = &(lib7_state->lib7_calleeSave[2]);
	    }
	} /* for */
    }
#else /* !MP_SUPPORT */
    *rootsPtr++ = &(lib7_state->lib7_link_register);
    *rootsPtr++ = &(lib7_state->lib7_argument);
    *rootsPtr++ = &(lib7_state->lib7_fate);
    *rootsPtr++ = &(lib7_state->lib7_closure);
    *rootsPtr++ = &(lib7_state->lib7_exception_fate);
    *rootsPtr++ = &(lib7_state->lib7_current_thread);
    *rootsPtr++ = &(lib7_state->lib7_calleeSave[0]);
    *rootsPtr++ = &(lib7_state->lib7_calleeSave[1]);
    *rootsPtr++ = &(lib7_state->lib7_calleeSave[2]);
#endif /* MP_SUPPORT */
    *rootsPtr = NULL;

    MinorGC (lib7_state, roots);

    heap = lib7_state->lib7_heap;

  /* Check for major GC */
    if (level == 0) {
	gen_t	*gen1 = heap->gen[0];
	Word_t	size = lib7_state->lib7_allocArenaSzB;

	for (i = 0;  i < NUM_ARENAS;  i++) {
	    arena_t *arena = gen1->arena[i];
	    if (isACTIVE(arena) && (AVAIL_SPACE(arena) < size)) {
		level = 1;
		break;
	    }
	}
    }

    if (level > 0) {
#ifdef MP_SUPPORT
	vproc_state_t   *vsp;
	lib7_state_t	*lib7_state;

	for (i = 0; i < MAX_NUM_PROCS; i++) {
	    vsp = VProc[i];
	    lib7_state = vsp->vp_state;
	    if (vsp->vp_mpState == MP_PROC_RUNNING)
		*rootsPtr++ = &(lib7_state->lib7_link_register);
	}
#else
	ASSIGN(ProfCurrent, PROF_MAJOR_GC);
#endif
	*rootsPtr = NULL;
	MajorGC (lib7_state, roots, level);
    }
    else {
	HeapMon_UpdateHeap (heap, 1);
    }

  /* reset the allocation space */
#ifdef MP_SUPPORT
    MP_FinishCollect (lib7_state, nProcs);
#else
    lib7_state->lib7_heap_cursor	= heap->allocBase;
#ifdef SOFT_POLL
    ResetPollLimit (lib7_state);
#else
    lib7_state->lib7_heap_limit    = HEAP_LIMIT(heap);
#endif
#endif

    STOP_GC_PAUSE();

    ASSIGN(ProfCurrent, PROF_RUNTIME);

}                                             /* collect_garbage */


void   collect_garbage_with_extra_roots   (lib7_state_t *lib7_state, int level, ...)
{
    /* Invoke a garbage collection with possible additional roots.  The list of
     * additional roots should be NULL terminated.  A garbage collection always
     * involves collecting the allocation space.  In addition, if level is greater
     * than 0, or if the first generation is full after the minor collection, then
     * a major collection of one or more generations is performed (at least level
     * generations are collected).
     *
     * NOTE: the MP version of this may be broken, since if a processor calls this
     * but isn't the collecting process, then the extra roots are lost.  XXX BUGGO FIXME
     */
    lib7_val_t	*roots[NUM_GC_ROOTS+NUM_EXTRA_ROOTS];	/* registers and globals */
    lib7_val_t	**rootsPtr = roots, *p;
    heap_t	*heap;
    int		i;
    va_list	ap;
#ifdef MP_SUPPORT
    int		nProcs;
#endif

    ASSIGN(ProfCurrent, PROF_MINOR_GC);

#ifdef MP_SUPPORT
#ifdef MP_DEBUG
    SayDebug ("igcwr %d\n", lib7_state->lib7_mpSelf);
#endif
    va_start (ap, level);
    nProcs = MP_StartCollectWithRoots (lib7_state, ap);
    va_end(ap);
    if (nProcs == 0)
	ASSIGN(ProfCurrent, PROF_RUNTIME);
	return; /* a waiting proc */
#endif

    START_GC_PAUSE(lib7_state->lib7_heap);

#ifdef C_CALLS
    *rootsPtr++ = &CInterfaceRootList;
#endif

#ifdef MP_SUPPORT
  /* get extra roots from procs that entered through collect_garbage_with_extra_roots.
   * Our extra roots were placed in mpExtraRoots by MP_StartCollectWithRoots.
   */
    for (i = 0; mpExtraRoots[i] != NULL; i++)
	*rootsPtr++ = mpExtraRoots[i];
#else
  /* record extra roots from param list */
    va_start (ap, level);
    while ((p = va_arg(ap, lib7_val_t *)) != NULL) {
	*rootsPtr++ = p;
    }
    va_end(ap);
#endif /* MP_SUPPORT */

  /* Gather the roots */
    for (i = 0;  i < NumCRoots;  i++)
	*rootsPtr++ = CRoots[i];
#ifdef MP_SUPPORT
    {
	lib7_state_t	*lib7_state;
	vproc_state_t   *vsp;
	int		j;
      
	for (j = 0; j < MAX_NUM_PROCS; j++) {
	    vsp = VProc[j];
	    lib7_state = vsp->vp_state;
#ifdef MP_DEBUG
	SayDebug ("lib7_state[%d] alloc/limit was %x/%x\n",
	    j, lib7_state->lib7_heap_cursor, lib7_state->lib7_heap_limit);
#endif
	    if (vsp->vp_mpState == MP_PROC_RUNNING) {
		*rootsPtr++ = &(lib7_state->lib7_argument);
		*rootsPtr++ = &(lib7_state->lib7_fate);
		*rootsPtr++ = &(lib7_state->lib7_closure);
		*rootsPtr++ = &(lib7_state->lib7_exception_fate);
		*rootsPtr++ = &(lib7_state->lib7_current_thread);
		*rootsPtr++ = &(lib7_state->lib7_calleeSave[0]);
		*rootsPtr++ = &(lib7_state->lib7_calleeSave[1]);
		*rootsPtr++ = &(lib7_state->lib7_calleeSave[2]);
	    }
	} /* for */
    }
#else /* !MP_SUPPORT */
    *rootsPtr++ = &(lib7_state->lib7_argument);
    *rootsPtr++ = &(lib7_state->lib7_fate);
    *rootsPtr++ = &(lib7_state->lib7_closure);
    *rootsPtr++ = &(lib7_state->lib7_exception_fate);
    *rootsPtr++ = &(lib7_state->lib7_current_thread);
    *rootsPtr++ = &(lib7_state->lib7_calleeSave[0]);
    *rootsPtr++ = &(lib7_state->lib7_calleeSave[1]);
    *rootsPtr++ = &(lib7_state->lib7_calleeSave[2]);
#endif /* MP_SUPPORT */
    *rootsPtr = NULL;

    MinorGC (lib7_state, roots);

    heap = lib7_state->lib7_heap;

  /* Check for major GC */
    if (level == 0) {
	gen_t	*gen1 = heap->gen[0];
	Word_t	size = lib7_state->lib7_allocArenaSzB;

	for (i = 0;  i < NUM_ARENAS;  i++) {
	    arena_t *arena = gen1->arena[i];
	    if (isACTIVE(arena) && (AVAIL_SPACE(arena) < size)) {
		level = 1;
		break;
	    }
	}
    }

    if (level > 0) {
#ifdef MP_SUPPORT
	vproc_state_t   *vsp;

	for (i = 0; i < MAX_NUM_PROCS; i++) {
	    vsp = VProc[i];
	    if (vsp->vp_mpState == MP_PROC_RUNNING)
		*rootsPtr++ = &(vsp->vp_state->lib7_link_register);
	}
#else
	ASSIGN(ProfCurrent, PROF_MAJOR_GC);
	*rootsPtr++ = &(lib7_state->lib7_link_register);
	*rootsPtr++ = &(lib7_state->lib7_program_counter);
#endif
	*rootsPtr = NULL;
	MajorGC (lib7_state, roots, level);
    }
    else {
	HeapMon_UpdateHeap (heap, 1);
    }

  /* reset the allocation space */
#ifdef MP_SUPPORT
    MP_FinishCollect (lib7_state, nProcs);
#else
    lib7_state->lib7_heap_cursor	= heap->allocBase;
#ifdef SOFT_POLL
    ResetPollLimit (lib7_state);
#else
    lib7_state->lib7_heap_limit    = HEAP_LIMIT(heap);
#endif
#endif

    STOP_GC_PAUSE();

    ASSIGN(ProfCurrent, PROF_RUNTIME);

}                                   /* collect_garbage_with_extra_roots */



bool_t   need_to_collect_garbage   (lib7_state_t *lib7_state, Word_t nbytes)
{
    /* Check to see if a GC is required, or if there is enough heap space for
     * nbytes worth of allocation.  Return TRUE, if GC is required, FALSE
     * otherwise.
     */

#if (defined(MP_SUPPORT) && defined(COMMENT_MP_GCPOLL))
    if ((((Addr_t)(lib7_state->lib7_heap_cursor)+nbytes) >= (Addr_t)(lib7_state->lib7_heap_limit))
    || (INT_LIB7toC(PollEvent) != 0))
#elif defined(MP_SUPPORT)
    if (((Addr_t)(lib7_state->lib7_heap_cursor)+nbytes) >= (Addr_t)(lib7_state->lib7_heap_limit))
#else
    if (((Addr_t)(lib7_state->lib7_heap_cursor)+nbytes) >= (Addr_t)HEAP_LIMIT(lib7_state->lib7_heap))
#endif
	return TRUE;
    else
	return FALSE;

}


#ifdef SOFT_POLL
/* ResetPollLimit:
 *
 * Reset the limit pointer according to the current polling frequency.
 */
void ResetPollLimit (lib7_state_t *lib7_state)
{
    int		pollFreq =  INT_LIB7toC(DEREF(PollFreq));	/* PollFreq is #defined in src/runtime/include/runtime-globals.h in terms of _PollFreq0 from src/runtime/main/globals.c  */
    heap_t*	heap     =  lib7_state->lib7_heap;

    /* Assumes lib7_heap_cursor has been reset:
    */
    lib7_state->lib7_real_heap_limit	= HEAP_LIMIT(heap);
    if (pollFreq > 0) {
	lib7_state->lib7_heap_limit  = heap->allocBase + pollFreq*POLL_GRAIN_CPSI;
	lib7_state->lib7_heap_limit  = (lib7_state->lib7_heap_limit > lib7_state->lib7_real_heap_limit)
	    ? lib7_state->lib7_real_heap_limit
	    : lib7_state->lib7_heap_limit;
    }
    else
	lib7_state->lib7_heap_limit  = lib7_state->lib7_real_heap_limit;

} /* end ResetPollLimit */
#endif /* SOFT_POLL */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
