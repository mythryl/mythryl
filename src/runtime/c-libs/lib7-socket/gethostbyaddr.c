/* gethostbyaddr.c
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

/*
###         "So many centuries after the Creation,
###          it is unlikely that anyone could find
###          hitherto unknown lands of any value."
###             -- report to King Ferdinand and
###                Queen Isabella of Spain, 1486
 */ 

/* _lib7_NetDB_gethostbyaddr
 *     : addr -> (String * String list * addr_family * addr list) option
 */
lib7_val_t _lib7_NetDB_gethostbyaddr (lib7_state_t *lib7_state, lib7_val_t arg)
{
    ASSERT (sizeof(struct in_addr) == GET_SEQ_LEN(arg));

    return _util_NetDB_mkhostent (
	lib7_state,
	gethostbyaddr (STR_LIB7toC(arg), sizeof(struct in_addr), AF_INET));
}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
