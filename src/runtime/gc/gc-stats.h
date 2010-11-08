/* gc-stats.h
 *
 */

#ifndef _GC_STATS_
#define _GC_STATS_

#include "stats-data.h"

/*
###             "Should array indices start at 0 or 1?
###              My compromise of 0.5 was rejected without,
###              I thought, proper consideration."
###
###                            -- Stan Kelly-Bootle
 */



#ifdef VM_STATS
extern void ReportVM (lib7_state_t *lib7_state, int maxCollectedGen);
#endif

#ifdef PAUSE_STATS

#define START_GC_PAUSE(HEAP)	{					\
	if (StatsOn) {							\
	    heap_t	    *__heap = (HEAP);				\
	    stat_rec_t	    *__p = &(StatsBuf[NStatsRecs]);		\
	    Unsigned32_t    __n = (Addr_t)(lib7_state->lib7_heap_cursor) - 		\
				    (Addr_t)(__heap->allocBase);	\
	    CNTR_INCR(&(__heap->numAlloc), __n);			\
	    __p->allocCnt = __heap->numAlloc;				\
	    __p->numGens = 0;						\
	    gettimeofday(&(__p->startTime), NULL);	\
	}								\
    }

#define NUM_GC_GENS(NGENS)		{				\
	if (StatsOn)							\
	    StatsBuf[NStatsRecs].numGens = (NGENS);			\
    }

#define STOP_GC_PAUSE()			{				\
	if (StatsOn) {							\
	    gettimeofday(&(StatsBuf[NStatsRecs].stopTime),		\
		NULL);				\
	    STATS_FINISH();						\
	}								\
    }

#else /* !PAUSE_STATS */
#define START_GC_PAUSE(HEAP)
#define NUM_GC_GENS(NGENS)
#define STOP_GC_PAUSE()
#endif /* PAUSE_STATS */

#endif


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
