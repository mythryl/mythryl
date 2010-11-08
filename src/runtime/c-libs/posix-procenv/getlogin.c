/* getlogin.c
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

/* _lib7_P_ProcEnv_getlogin: Void -> String
 *
 * Return login name
 */
lib7_val_t   _lib7_P_ProcEnv_getlogin   (lib7_state_t *lib7_state, lib7_val_t arg)
{
    char* name = getlogin ();

    if (name == NULL)
        return RAISE_ERROR(lib7_state, "no login name");
  
    return LIB7_CString (lib7_state, name);
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
