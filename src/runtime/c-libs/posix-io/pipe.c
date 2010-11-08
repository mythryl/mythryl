/* pipe.c
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

/* _lib7_P_IO_pipe : Void -> int * int
 *
 * Create a pipe and return its input and output descriptors.
 */
lib7_val_t _lib7_P_IO_pipe (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int         fds[2];

    if (pipe(fds) == -1) {

        return RAISE_SYSERR(lib7_state, -1);

    } else {

        lib7_val_t        chunk;
        REC_ALLOC2 (lib7_state, chunk, INT_CtoLib7(fds[0]), INT_CtoLib7(fds[1]));
        return chunk;
    }

} /* end of _lib7_P_IO_pipe */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
