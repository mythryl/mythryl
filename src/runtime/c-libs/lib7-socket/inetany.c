/* inetany.c
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
###           "Radio has no future."
###                   -- Lord Kelvin, 1897
*/



/* _lib7_Sock_inetany : int -> addr
 *
 * Make an INET_ANY INET socket address, with the given port ID.
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/internet-socket.pkg
 */
lib7_val_t _lib7_Sock_inetany (lib7_state_t *lib7_state, lib7_val_t arg)
{
    struct sockaddr_in	addr;
    lib7_val_t		data, res;

    memset(&addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(INT_LIB7toC(arg));

    data = LIB7_CData (lib7_state, &addr, sizeof(struct sockaddr_in));
    SEQHDR_ALLOC (lib7_state, res, DESC_word8vec, data, sizeof(struct sockaddr_in));

    return res;

} /* end of _lib7_Sock_inetany */



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
