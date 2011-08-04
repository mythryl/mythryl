// cfun-list.h
//
// C functions callable from Mythryl.
//
// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"ncurses"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"November 29, 2007"
#endif

CFUNC("initscr","initscr",	_lib7_Ncurses_initscr,		"Void -> Void")
CFUNC("nl","nl",		_lib7_Ncurses_nl,		"Void -> Void")
CFUNC("nonl","nonl",		_lib7_Ncurses_nonl,		"Void -> Void")
CFUNC("cbreak","cbreak",		_lib7_Ncurses_cbreak,		"Void -> Void")
CFUNC("noecho","noecho",		_lib7_Ncurses_noecho,		"Void -> Void")
CFUNC("start_color","start_color",	_lib7_Ncurses_start_color,	"Void -> Void")
CFUNC("endwin","endwin",		_lib7_Ncurses_endwin,		"Void -> Void")
CFUNC("refresh","refresh",	_lib7_Ncurses_refresh,		"Void -> Void")
CFUNC("has_colors","has_colors",	_lib7_Ncurses_has_colors,	"Void -> Bool")
CFUNC("getch","getch",		_lib7_Ncurses_getch,		"Void -> Char")
CFUNC("addch","addch",		_lib7_Ncurses_addch,		"Char -> Void")
CFUNC("move","move",		_lib7_Ncurses_move,		"(Int, Int) -> Void")




// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

