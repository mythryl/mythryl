/* from-inetaddr.c
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


/*
###          "The first 90% of the code accounts for
###           the first 90% of the development time.
###           The remaining 10% of the code accounts for
###           the other 90% of the development time."
###
###                            -- Tom Cargill
 */


/* _lib7_Sock_frominetaddr : addr -> (in_addr * int)
 *
 * Given a INET-domain socket address, return the INET address and port number.
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/internet-socket.pkg
 */
lib7_val_t _lib7_Sock_frominetaddr (lib7_state_t *lib7_state, lib7_val_t arg)
{
    struct sockaddr_in	*addr = GET_SEQ_DATAPTR(struct sockaddr_in, arg);
    lib7_val_t		data, inAddr, res;

    ASSERT (addr->sin_family == AF_INET);

    data = LIB7_CData (lib7_state, &(addr->sin_addr), sizeof(struct in_addr));
    SEQHDR_ALLOC (lib7_state, inAddr, DESC_word8vec, data, sizeof(struct in_addr));
    REC_ALLOC2 (lib7_state, res, inAddr, INT_CtoLib7(ntohs(addr->sin_port)));

    return res;

} /* end of _lib7_Sock_frominetaddr */



/* COPYRIGHT (c) 1996 AT&T Research.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
