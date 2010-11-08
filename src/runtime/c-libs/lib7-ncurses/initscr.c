/* initscr.c
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

/* _lib7_Ncurses_initscr : Void -> Void
 *
 * According to the ncurses(3NCURSES) manpage: 
 *      To  initialize  the  routines,  the  routine initscr or newterm must be
 *      called before any of the other routines  that  deal  with  windows  and
 *      screens  are  used.   The routine endwin must be called before exiting.
 *      To get character-at-a-time input  without  echoing  (most  interactive,
 *      screen  oriented  programs want this), the following sequence should be
 *      used:
 *
 *             initscr(); cbreak(); noecho();
 *
 *      Most programs would additionally use the sequence:
 *
 *            nonl();
 *            intrflush(stdscr, FALSE);
 *            keypad(stdscr, TRUE);
 *
 */
lib7_val_t

_lib7_Ncurses_initscr (lib7_state_t *lib7_state, lib7_val_t arg)
{
#if HAVE_CURSES_H && HAVE_LIBNCURSES
    initscr();
    return LIB7_void;
#else
    extern char* no_ncurses_support_in_runtime;
    return RAISE_ERROR(lib7_state, no_ncurses_support_in_runtime);
#endif
}


/* Code by Jeff Prothero: Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
