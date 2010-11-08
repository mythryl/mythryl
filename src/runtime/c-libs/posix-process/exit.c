/* exit.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

/*
###     "We pray for one last landing
###        on the globe that gave us birth;
###      Let us rest our eyes on the fleecy skies
###        and the cool, green hills of Earth."
###
###                  -- "Noisy" Rhysling
 */

/* _lib7_P_Process_exit : int -> 'a
 *
 * Exit from process
 */
lib7_val_t _lib7_P_Process_exit (lib7_state_t *lib7_state, lib7_val_t arg)
{
    Exit (INT_LIB7toC(arg));

    /*NOTREACHED*/
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
