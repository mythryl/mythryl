/* getenv.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"
#include <stdio.h>

/* _lib7_P_ProcEnv_getenv: String -> String option
 *
 * Return value for environment name
 */
lib7_val_t _lib7_P_ProcEnv_getenv (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t  r, s;

    char* status = getenv( STR_LIB7toC(arg) );

    if (status == NULL) {
        return OPTION_NONE;
    }

    s = LIB7_CString( lib7_state, status);
    OPTION_SOME(lib7_state, r, s)
  
    return r;
}



/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
