/* getaddrfamily.c
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

/* _lib7_Sock_getaddrfamily : addr -> af
 *
 * Extract the family field, convert to host byteorder, and return it.
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t _lib7_Sock_getaddrfamily (lib7_state_t *lib7_state, lib7_val_t arg)
{
    struct sockaddr *addr = GET_SEQ_DATAPTR(struct sockaddr, arg);

    return LIB7_SysConst (lib7_state, &_Sock_AddrFamily, ntohs(addr->sa_family));

} /* end of _lib7_Sock_getaddrfamily */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
