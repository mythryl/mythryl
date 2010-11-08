/* alloc-code.c
 *
 */

#include "../../config.h"

#include "cache-flush.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-state.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"


lib7_val_t   _lib7_runtime_alloc_code   (   lib7_state_t*   lib7_state,
                                              lib7_val_t      arg
                                          )
{
    /* _lib7_runtime_alloc_code : int -> rw_unt8_vector.Rw_Vector
     *
     * Allocate a code chunk of the given size.
     *
     * Note: Generating the name string within the code chunk
     *       the code generator's responsibility.
     */

    int	          nbytes =   INT_LIB7toC( arg );

    lib7_val_t	  code   =   LIB7_AllocCode( lib7_state, nbytes );

    {   lib7_val_t	          result;
        SEQHDR_ALLOC(lib7_state, result, DESC_word8arr, code, nbytes);
        return                    result;
    }
}


/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
