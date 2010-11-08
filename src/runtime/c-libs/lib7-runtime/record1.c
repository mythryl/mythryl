/* record1.c
 *
 * Create a singleton record.
 */


/*
###               "He travels the fastest who travels alone."
###
###                                 -- Rudyard Kipling
 */


#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-state.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"



lib7_val_t   _lib7_runtime_record1   (   lib7_state_t*   lib7_state,
                                           lib7_val_t      arg
                                       )
{
    /* _lib7_runtime_record1 : chunk -> chunk */

    lib7_val_t    result;

    REC_ALLOC1( lib7_state, result, arg );

    return result;
}



/* COPYRIGHT (c) 1998 Bell Labs, Lucent Technologies.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

