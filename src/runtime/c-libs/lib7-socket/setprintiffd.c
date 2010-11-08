/* setprintiffd.c
 *
 */

#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#include "print-if.h"

/* _lib7_Sock_setprintiffd : Int -> Void
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/internet-socket.pkg
 */
lib7_val_t _lib7_Sock_setprintiffd (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int fd      =  INT_LIB7toC(arg);

    print_if_fd = fd;

    return LIB7_void;
}


/* COPYRIGHT (c) 2010 by Jeff Prothero,
 * released under Gnu Public Licence version 3.
 */
