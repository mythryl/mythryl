/* gc-ctl.c
 *
 * General interface for GC control functions.
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-state.h"
#include "memory.h"
#include "heap.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"

#define STREQ(s1, s2)	(strcmp((s1), STR_LIB7toC(s2)) == 0)

static void SetVMCache (lib7_state_t *lib7_state, lib7_val_t cell);
static void DoGC (lib7_state_t *lib7_state, lib7_val_t cell, lib7_val_t *next);
static void AllGC (lib7_state_t *lib7_state, lib7_val_t *next);



lib7_val_t   _lib7_runtime_gc_ctl   (   lib7_state_t*   lib7_state,
                                          lib7_val_t      arg
                                      )
{
    /* _lib7_runtime_gc_ctl : (String * int ref) list -> Void
     *
     * Current control operations:
     *
     *   ("SetVMCache", ref n)	- sets VM cache level to n; returns old cache
     *				  level.
     *   ("DoGC", ref n)		- does a GC of the first "n" generations
     *   ("AllGC", _)		- collects all generations.
     *   ("Messages", ref 0)	- turn GC messages off
     *   ("Messages", ref n)	- turn GC messages on (n > 0)
     */

    while (arg != LIST_nil) {

	lib7_val_t	cmd = LIST_hd(arg);
	lib7_val_t	oper = REC_SEL(cmd, 0);
	lib7_val_t	cell = REC_SEL(cmd, 1);

	arg = LIST_tl(arg);

	if (STREQ("SetVMCache", oper))	    SetVMCache (lib7_state, cell);
	else if (STREQ("DoGC", oper))	    DoGC (lib7_state, cell, &arg);
	else if (STREQ("AllGC", oper))	    AllGC (lib7_state, &arg);
	else if (STREQ("Messages", oper))   GCMessages    = (INT_LIB7toC(DEREF(cell)) > 0);
	else if (STREQ("LimitHeap", oper))  UnlimitedHeap = (INT_LIB7toC(DEREF(cell)) <= 0);
    }

    return LIB7_void;
}


static void   SetVMCache   (   lib7_state_t*   lib7_state,
                               lib7_val_t      arg
                           )
{
    /* Set the VM cache generation, return the old level. */

    int		level = INT_LIB7toC(DEREF(arg));
    heap_t	*heap = lib7_state->lib7_heap;

    if (level < 0)
	level = 0;
    else if (level > MAX_NUM_GENS)
	level = MAX_NUM_GENS;

    if (level < heap->cacheGen) {
      /* Free any cached memory chunks. */
	int		i;
	for (i = level;  i < heap->cacheGen;  i++)
	    free_heap_chunk (heap->gen[i]->cacheChunk);
    }

    ASSIGN(arg, INT_CtoLib7(heap->cacheGen));
    heap->cacheGen = level;
}


static void   DoGC   (   lib7_state_t*   lib7_state,
                         lib7_val_t      arg,
                         lib7_val_t*     next
                     )
{
    /* Force a garbage collection of the given level. */

    heap_t	*heap = lib7_state->lib7_heap;
    int		level = INT_LIB7toC(DEREF(arg));

    if (level < 0)
	level = 0;
    else if (heap->numGens < level)
	level = heap->numGens;

    collect_garbage_with_extra_roots (lib7_state, level, next, NULL);
}



static void   AllGC   (   lib7_state_t*   lib7_state,
                          lib7_val_t*     next
                      )
{
    /* Force a garbage collection of all generations: */

    collect_garbage_with_extra_roots(   lib7_state,
                                        lib7_state->lib7_heap->numGens,
                                        next,
                                        NULL
                                    );
}



/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

