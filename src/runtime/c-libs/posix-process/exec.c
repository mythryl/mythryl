/* exec.c
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

/* _lib7_P_Process_exec : String * String list -> 'a
 *
 * Overlay a new process image
 */
lib7_val_t _lib7_P_Process_exec (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int             status;
    lib7_val_t	    path = REC_SEL(arg, 0);
    lib7_val_t        arglst = REC_SEL(arg, 1);
    char            **argv;
    lib7_val_t        p;
    char            **cp;

      /* use the heap for temp space for the argv[] vector */
    cp = (char **)(lib7_state->lib7_heap_cursor);
#ifdef SIZES_C64_LIB732
      /* must 8-byte align this */
    cp = (char **)ROUNDUP((Unsigned64_t)cp, ADDR_SZB);
#endif
    argv = cp;
    for (p = arglst;  p != LIST_nil;  p = LIST_tl(p))
        *cp++ = STR_LIB7toC(LIST_hd(p));
    *cp++ = 0;  /* terminate the argv[] */

    status = execv(STR_LIB7toC(path), argv);
    CHECK_RETURN (lib7_state, status)

} /* end of _lib7_P_Process_exec */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
