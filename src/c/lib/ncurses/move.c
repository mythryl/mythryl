// move.c

#include "../../mythryl-config.h"

#if HAVE_CURSES_H
#include <curses.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"
#include "../raise-error.h"



Val   _lib7_Ncurses_move   (Task* task,  Val arg)   {    // : (Int, Int) -> Void
    //==================
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Ncurses_move");

    #if HAVE_CURSES_H && HAVE_LIBNCURSES
	int y = INT1_LIB7toC( GET_TUPLE_SLOT_AS_INT(arg, 0) );
	int x = INT1_LIB7toC( GET_TUPLE_SLOT_AS_INT(arg, 1) );

	int result = move( y, x );

	if (result == ERR)     return RAISE_ERROR__MAY_HEAPCLEAN(task, "move", NULL);

	return HEAP_VOID;
    #else
	extern char* no_ncurses_support_in_runtime;
	//
	return RAISE_ERROR__MAY_HEAPCLEAN(task, no_ncurses_support_in_runtime, NULL);
    #endif
}


// Code by Jeff Prothero: Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

