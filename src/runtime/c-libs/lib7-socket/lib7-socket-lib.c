/* lib7-socket-lib.c
 *
 */

#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "c-library.h"
#include "cfun-proto-list.h"

/*
###         "In a room full of top software designers, if
###          any two of them agree -- that's a majority!"
###
###                                -- Bill Curtis
 */

/*/* The table of C functions and Lib7 names */
#define CFUNC(NAME, FUNC, LIB7TYPE)	CFUNC_BIND(NAME, FUNC, LIB7TYPE)
static cfunc_naming_t CFunTable[] = {
#include "cfun-list.h"
	CFUNC_NULL_BIND
    };
#undef CFUNC


void init_g(int argc, char **argv)
{
#if defined(OPSYS_WIN32)
  static int nCode = -1;
  if( nCode!=0 )
    {
      WSADATA wsaData;
      nCode = WSAStartup(MAKEWORD(1, 1), &wsaData);
      /* FIXME: what to do if WSAStartup fails (nCode!=0)? */
    }
#endif
}


/* the Sockets library */
c_library_t	    Lib7_Sock_Library = {
	CLIB_NAME,
	CLIB_VERSION,
	CLIB_DATE,
        init_g,
	CFunTable
    };



/* COPYRIGHT (c) 1994 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
