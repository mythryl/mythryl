/* mkliterals.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-state.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"



lib7_val_t   _lib7_runtime_mkliterals   (   lib7_state_t*   lib7_state,
                                              lib7_val_t      arg
                                          )
{
    /* _lib7_runtime_mkliterals : unt8_vector.vector -> chunk vector */

    return BuildLiterals (lib7_state, GET_SEQ_DATAPTR(Byte_t, arg), GET_SEQ_LEN(arg));
}


/* COPYRIGHT (c) 1998 Bell Labs, Lucent Technologies.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
