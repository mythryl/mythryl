/* getnetbyname.c
 *
 */

#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "socket-util.h"

#if defined(__CYGWIN32__)
#undef getnetbyname
#define getnetbyname(x) NULL
#endif

/*
###             "Any two friends living within the radius of sensibility
###              of their [electrical ray] receiving instruments, having
###              first decided on their special wave length and attuned
###              their respective instruments to mutual receptivity,
###              could thus communicate as long and as often as they
###              pleased by timing the impulses to produce long and short
###              intervals on the ordinary Morse code."
###
###                                    -- William Crookes, 1892
 */


/* _lib7_NetDB_getnetbyname : String -> (String * String list * addr_family * sysword) option
 */
lib7_val_t _lib7_NetDB_getnetbyname (lib7_state_t *lib7_state, lib7_val_t arg)
{
#if defined(OPSYS_WIN32)
  /* FIXME:  getnetbyname() does not seem to exist under Windows.  What is
     the equivalent? */
  return RAISE_ERROR(lib7_state, "<getnetbyname not implemented>");
#else
    return _util_NetDB_mknetent (lib7_state, getnetbyname (STR_LIB7toC(arg)));
#endif
} /* end of _lib7_NetDB_getnetbyname */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
