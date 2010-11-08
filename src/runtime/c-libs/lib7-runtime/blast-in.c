/* blast_in.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "heap-io.h"
#include "cfun-proto-list.h"

lib7_val_t   _lib7_runtime_blast_in   (   lib7_state_t*   lib7_state,
                                            lib7_val_t      arg
                                        )
{
    /* _lib7_runtime_blast_in : String -> 'a
     *
     * Build a Lib7 chunk from a string.
     */

    bool_t	seen_error = FALSE;

    lib7_val_t chunk = BlastIn (lib7_state, PTR_LIB7toC(Byte_t, arg), CHUNK_LEN(arg), &seen_error);

    if (seen_error)  	return RAISE_ERROR( lib7_state, "blast_in");
    else         	return chunk;
}



/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
