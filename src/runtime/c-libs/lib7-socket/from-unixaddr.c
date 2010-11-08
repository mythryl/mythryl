/* from-unixaddr.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"
#include "sockets-osdep.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include INCLUDE_SOCKET_H
#include INCLUDE_UN_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"


/*
###           "If builders built buildings the
###            way programmers write programs,
###            then the first woodpecker that came
###            along would destroy civilization."
###
###                       -- Gerald M Weinberg
 */


/* _lib7_Sock_fromunixaddr : addr -> String
 *
 * Given a UNIX-domain socket address, return the string.
 */
lib7_val_t _lib7_Sock_fromunixaddr (lib7_state_t *lib7_state, lib7_val_t arg)
{
    struct sockaddr_un	*addr = GET_SEQ_DATAPTR(struct sockaddr_un, arg);

    ASSERT(addr->sun_family == AF_UNIX);

    return LIB7_CString(lib7_state, addr->sun_path);

} /* end of _lib7_Sock_fromunixaddr */



/* COPYRIGHT (c) 1996 AT&T Research.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
