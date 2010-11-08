/* mkexec.c
 *
 */

#include "../../config.h"

#include "cache-flush.h"
#include "runtime-base.h"
#include "runtime-heap.h"
#include "runtime-state.h"
#include "cfun-proto-list.h"


lib7_val_t   _lib7_runtime_mkexec   (   lib7_state_t*   lib7_state,
                                          lib7_val_t      arg
                                      )
{
    /* _lib7_runtime_mkexec : rw_unt8_vector.Rw_Vector * int -> (chunk -> chunk)
     *
     * Turn a previously allocated code chunk into a closure.
     * This requires that we flush the I-cache.
     */

    lib7_val_t   seq        = REC_SEL(   arg, 0);
    int           entrypoint = REC_SELINT(arg, 1);

    char*	  code       = GET_SEQ_DATAPTR( char, seq );
    Word_t	  nbytes     = GET_SEQ_LEN(           seq );

    FlushICache (code, nbytes);

    {   lib7_val_t	        result;
        REC_ALLOC1(lib7_state, result, PTR_CtoLib7(code + entrypoint));
        return                  result;
    }
}



/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
