/* to-unixaddr.c
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
#include "lib7-c.h"
#include "cfun-proto-list.h"


/* _lib7_Sock_tounixaddr : String -> addr
 *
 * Given a path, allocate a UNIX-domain socket address.
 */
lib7_val_t _lib7_Sock_tounixaddr (lib7_state_t *lib7_state, lib7_val_t arg)
{
    char		*path = STR_LIB7toC(arg);
    struct sockaddr_un	addr;
    int			len;
    lib7_val_t		data, res;

    memset(&addr, 0, sizeof(struct sockaddr_un));

    addr.sun_family = AF_UNIX;
    strcpy (addr.sun_path, path);
#ifdef SOCKADDR_HAS_LEN
    len = strlen(path)+sizeof(addr.sun_len)+sizeof(addr.sun_family)+1;
    addr.sun_len = len;
#else
    len = strlen(path)+sizeof(addr.sun_family)+1;
#endif

    data = LIB7_CData (lib7_state, &addr, len);
    SEQHDR_ALLOC (lib7_state, res, DESC_word8vec, data, len);

    return res;

} /* end of _lib7_Sock_tounixaddr */

/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
