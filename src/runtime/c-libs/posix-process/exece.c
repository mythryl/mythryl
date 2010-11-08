/* exece.c
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

/* _lib7_P_Process_exece : String * String list * String list -> 'a
 *
 * Overlay a new process image, using specified environment.
 */
lib7_val_t _lib7_P_Process_exece (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int             status;
    lib7_val_t      path   = REC_SEL(arg, 0);
    lib7_val_t      arglst = REC_SEL(arg, 1);
    lib7_val_t	    envlst = REC_SEL(arg, 2);
    char            **argv, **envp;
    lib7_val_t        p;
    char            **cp;

      /* use the heap for temp space for the argv[] and envp[] vectors */
    cp = (char **)(lib7_state->lib7_heap_cursor);
#ifdef SIZES_C64_LIB732
      /* must 8-byte align this */
    cp = (char **)ROUNDUP((Unsigned64_t)cp, ADDR_SZB);
#endif
    argv = cp;
    for (p = arglst;  p != LIST_nil;  p = LIST_tl(p))
        *cp++ = STR_LIB7toC(LIST_hd(p));
    *cp++ = 0;  /* terminate the argv[] */

    envp = cp;
    for (p = envlst;  p != LIST_nil;  p = LIST_tl(p))
        *cp++ = STR_LIB7toC(LIST_hd(p));
    *cp++ = 0;  /* terminate the envp[] */

    status = execve(STR_LIB7toC(path), argv, envp);

    CHECK_RETURN (lib7_state, status)
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
