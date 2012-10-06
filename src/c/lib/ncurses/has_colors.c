// has_colors.c

#include "../../mythryl-config.h"

#if HAVE_CURSES_H
#include <curses.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"
#include "../raise-error.h"



Val   _lib7_Ncurses_has_colors   (Task* task,  Val arg)   {	// : Void -> Bool
    //========================
    //
    Val result;
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    #if HAVE_CURSES_H && HAVE_LIBNCURSES
	result = (has_colors()  ? HEAP_TRUE : HEAP_FALSE);
    #else
	extern char* no_ncurses_support_in_runtime;
	//
	result = RAISE_ERROR__MAY_HEAPCLEAN(task, no_ncurses_support_in_runtime, NULL);
    #endif
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}



// Code by Jeff Prothero: Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

