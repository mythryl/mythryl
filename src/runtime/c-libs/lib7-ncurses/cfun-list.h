/* cfun-list.h
 *
 *
 * This file lists the directory library of C functions that are callable by lib7.
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"Lib7-Ncurses"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"November 29, 2007"
#endif

CFUNC("initscr",	_lib7_Ncurses_initscr,		"Void -> Void")
CFUNC("nl",		_lib7_Ncurses_nl,		"Void -> Void")
CFUNC("nonl",		_lib7_Ncurses_nonl,		"Void -> Void")
CFUNC("cbreak",		_lib7_Ncurses_cbreak,		"Void -> Void")
CFUNC("noecho",		_lib7_Ncurses_noecho,		"Void -> Void")
CFUNC("start_color",	_lib7_Ncurses_start_color,	"Void -> Void")
CFUNC("endwin",		_lib7_Ncurses_endwin,		"Void -> Void")
CFUNC("refresh",	_lib7_Ncurses_refresh,		"Void -> Void")
CFUNC("has_colors",	_lib7_Ncurses_has_colors,	"Void -> Bool")
CFUNC("getch",		_lib7_Ncurses_getch,		"Void -> Char")
CFUNC("addch",		_lib7_Ncurses_addch,		"Char -> Void")
CFUNC("move",		_lib7_Ncurses_move,		"(Int, Int) -> Void")




/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
