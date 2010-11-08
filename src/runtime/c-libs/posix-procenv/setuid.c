/* setuid.c
 *
 */

#include "../../config.h"

#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

/* _lib7_P_ProcEnv_setuid: word -> Void
 *
 * Set user id
 */
lib7_val_t _lib7_P_ProcEnv_setuid (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int status = setuid(WORD_LIB7toC(arg));

    CHECK_RETURN_UNIT(lib7_state, status)
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
