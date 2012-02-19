// start_color.c

#include "../../mythryl-config.h"

#if HAVE_CURSES_H
#include <curses.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"
#include "../raise-error.h"

/* _lib7_Ncurses_start_color : Void -> Void
 *
 */
Val

_lib7_Ncurses_start_color (Task *task, Val arg)
{
#if HAVE_CURSES_H && HAVE_LIBNCURSES
    start_color();
    return HEAP_VOID;
#else
    extern char* no_ncurses_support_in_runtime;
    //
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_ncurses_support_in_runtime, NULL);
#endif
}


// Code by Jeff Prothero: Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


