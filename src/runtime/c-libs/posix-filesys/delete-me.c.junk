/* tracing.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

void print_if_lib7_P_FileSys_openf (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	    path  = REC_SEL(arg, 0);
    int		    flags = REC_SELWORD(arg, 1);
    int		    mode  = REC_SELWORD(arg, 2);

    int		    fd    = open (STR_LIB7toC(path), flags, mode);

    CHECK_RETURN(lib7_state, fd)

} /* end of _lib7_P_FileSys_openf */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
