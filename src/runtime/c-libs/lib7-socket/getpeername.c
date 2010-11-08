/* getpeername.c
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
###                 "Stocks have reached what looks like a a permanently high plateau."
###
###                      -- Irving Fisher, Professor of Economics, Yale University, 1929
*/



/* _lib7_Sock_getpeername : Socket -> (Address_Family, Address)
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t

_lib7_Sock_getpeername (lib7_state_t *lib7_state, lib7_val_t arg)
{
    char	    addr[MAX_SOCK_ADDR_SZB];
    int		    addrLen = MAX_SOCK_ADDR_SZB;

    if (getpeername (INT_LIB7toC(arg), (struct sockaddr *)addr, &addrLen) < 0) {

        return RAISE_SYSERR(lib7_state, status);

    } else {

	lib7_val_t	cdata = LIB7_CData(lib7_state, addr, addrLen);
	lib7_val_t	res;

	SEQHDR_ALLOC (lib7_state, res, DESC_word8vec, cdata, addrLen);
	return res;
    }
}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
