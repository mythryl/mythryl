/* nl.c
 *
 */

#include "../../config.h"

#if HAVE_CURSES_H
#include <curses.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"
#include "../lib7-c.h"

/* _lib7_Ncurses_nl : Void -> Void
 *
 */
lib7_val_t

_lib7_Ncurses_nl (lib7_state_t *lib7_state, lib7_val_t arg)
{
#if HAVE_CURSES_H && HAVE_LIBNCURSES
    nl();
    return LIB7_void;
#else
    extern char* no_ncurses_support_in_runtime;
    return RAISE_ERROR(lib7_state, no_ncurses_support_in_runtime);
#endif
}

/* Code by Jeff Prothero: Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
