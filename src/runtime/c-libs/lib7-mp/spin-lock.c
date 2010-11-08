/* spin-lock.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-state.h"
#include "runtime-heap.h"
#include "runtime-mp.h"
#include "cfun-proto-list.h"


/* _lib7_MP_spin_lock:
 */
lib7_val_t

_lib7_MP_spin_lock (lib7_state_t *lib7_state, lib7_val_t arg)
{
#ifdef MP_SUPPORT
    /* this code is for use the assembly (MIPS.prim.asm) try_lock and lock */
    lib7_val_t r;

    REF_ALLOC(lib7_state, r, LIB7_false);
    return r;
#else
    Die ("lib7_spin_lock: no mp support\n");
#endif

}


/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
