// initscr.c

#include "../../mythryl-config.h"

#if HAVE_CURSES_H
#include <curses.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"
#include "../raise-error.h"



// According to the ncurses(3NCURSES) manpage: 
//      To  initialize  the  routines,  the  routine initscr or newterm must be
//      called before any of the other routines  that  deal  with  windows  and
//      screens  are  used.   The routine endwin must be called before exiting.
//      To get character-at-a-time input  without  echoing  (most  interactive,
//      screen  oriented  programs want this), the following sequence should be
//      used:
//
//             initscr(); cbreak(); noecho();
//
//      Most programs would additionally use the sequence:
//
//            nonl();
//            intrflush(stdscr, FALSE);
//            keypad(stdscr, TRUE);
//
Val   _lib7_Ncurses_initscr   (Task* task,  Val arg)   {	// : Void -> Void
    //=====================
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Ncurses_initscr");

    #if HAVE_CURSES_H && HAVE_LIBNCURSES
	initscr();
	return HEAP_VOID;
    #else
	extern char* no_ncurses_support_in_runtime;
	//
	return  RAISE_ERROR__MAY_HEAPCLEAN(task, no_ncurses_support_in_runtime, NULL);
    #endif
}


// Code by Jeff Prothero: Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

