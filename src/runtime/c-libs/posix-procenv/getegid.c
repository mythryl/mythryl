/* getegid.c
 *
 */

#include "../../config.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "runtime-heap.h"
#include "cfun-proto-list.h"

/* _lib7_P_ProcEnv_getegid: Void -> word
 *
 * Return effective group id
 */
lib7_val_t _lib7_P_ProcEnv_getegid (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	p;

    WORD_ALLOC (lib7_state, p, (Word_t)(getegid()));

    return p;
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
