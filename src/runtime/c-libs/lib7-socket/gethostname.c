/* gethostname.c
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

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

/*
###           "Well informed people know it is
###            impossible to transmit the voice
###            over wires and that were it possible
###            to do so, the thing would be of no
###            practical value."
###
###                     -- Boston Post, 1865
 */

/* _lib7_NetDB_gethostname : Void -> String
 */
lib7_val_t

_lib7_NetDB_gethostname (lib7_state_t *lib7_state, lib7_val_t arg)
{
    char hostname[ MAXHOSTNAMELEN ];

    if (gethostname (hostname, MAXHOSTNAMELEN) == -1)  return RAISE_SYSERR(lib7_state, status);
    else	                                       return LIB7_CString(lib7_state, hostname);
}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
