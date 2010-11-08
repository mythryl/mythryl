/* getnetbyaddr.c
 *
 */

#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "socket-util.h"

#if defined(__CYGWIN32__)
#undef getnetbyaddr
#define getnetbyaddr(x,y) NULL
#endif


/*
###                  "This 'telephone' has too many shortcomings
###                   to be seriously considered as a means of
###                   communication. The device is inherently
###                   of no value to us."
###
###                             -- Western Union memo, 1865
 */


/* _lib7_NetDB_getnetbyaddr
 *     : (Sysword, Address_Family) -> Null_Or(String, List( String ), Address_Family, Sysword)
 */
lib7_val_t

_lib7_NetDB_getnetbyaddr (lib7_state_t *lib7_state, lib7_val_t arg)
{
#if defined(OPSYS_WIN32)
  /* FIXME:  getnetbyaddr() does not seem to exist under Windows.  What is
     the equivalent? */
  return RAISE_ERROR(lib7_state, "<getnetbyaddr not implemented>");
#else
    unsigned long   net = REC_SELWORD(arg, 0);
    int		    type = REC_SELINT(arg, 1);

    return _util_NetDB_mknetent (lib7_state, getnetbyaddr(net, type));
#endif
}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
