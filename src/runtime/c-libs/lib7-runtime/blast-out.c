/* blast_out.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "heap-io.h"
#include "cfun-proto-list.h"

lib7_val_t   _lib7_runtime_blast_out   (   lib7_state_t*   lib7_state,
                                             lib7_val_t      arg
                                         )
{
    /* _lib7_runtime_blast_out : 'a -> unt8_vector.Vector
     *
     * Translate a heap chunk into a linear representation (vector of bytes).
     */

    lib7_val_t	data;

    data = BlastOut (lib7_state, arg);

    if (data == LIB7_void)   return RAISE_ERROR(lib7_state, "attempt to blast chunk failed");
    else                     return data;
}



/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
