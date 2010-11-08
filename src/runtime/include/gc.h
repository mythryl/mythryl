/* gc.h
 *
 * The external interface to the garbage collector.
 *
 */

#ifndef _GC_
#define _GC_

#ifndef _LIB7_BASE_
#include "runtime-base.h"
#endif

/* typedef struct heap heap_t; */	/* from runtime-base.h */

extern void      set_up_heap                           (lib7_state_t *lib7_state, bool_t is_boot, heap_params_t *params);
extern void      collect_garbage                    (lib7_state_t *lib7_state, int level);
extern void      collect_garbage_with_extra_roots   (lib7_state_t *lib7_state, int level, ...);
extern bool_t    need_to_collect_garbage            (lib7_state_t *lib7_state, Word_t nbytes);

extern int GetChunkGen (lib7_val_t chunk);
extern lib7_val_t RecordMeld (lib7_state_t *lib7_state, lib7_val_t r1, lib7_val_t r2);

char *BO_AddrToCodeChunkTag (Word_t pc);

#ifdef HEAP_MONITOR
extern status_t set_up_heap_monitor (heap_t *heap);
#else
#define set_up_heap_monitor(A)
#endif

#ifdef GC_STATS
extern void ClearGCStats (heap_t *heap);
#endif

#endif /* !_GC_ */


/* COPYRIGHT (c) 1992 AT&T Bell Laboratories
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

