/* environ.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"

/* _lib7_P_ProcEnv_environ : Void -> String list
 */
lib7_val_t _lib7_P_ProcEnv_environ (lib7_state_t *lib7_state, lib7_val_t arg)
{
    extern char         **environ;

    return LIB7_CStringList (lib7_state, environ);
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
