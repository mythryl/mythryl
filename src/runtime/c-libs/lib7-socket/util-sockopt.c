/* util-sockopt.c
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

/* _util_Sock_ControlFlg:
 *
 * This utility routine gets/sets a boolean socket option.
 */
lib7_val_t _util_Sock_ControlFlg (lib7_state_t *lib7_state, lib7_val_t arg, int option)
{
    int		socket = REC_SELINT(arg, 0);
    lib7_val_t	ctl = REC_SEL(arg, 1);
    int		flag, status;

    if (ctl == OPTION_NONE) {
	int	optSz = sizeof(int);
	status = getsockopt (socket, SOL_SOCKET, option, (sockoptval_t)&flag, &optSz);
	ASSERT((status < 0) || (optSz == sizeof(int)));
    }
    else {
	flag = INT_LIB7toC(OPTION_get(ctl));
	status = setsockopt (socket, SOL_SOCKET, option, (sockoptval_t)&flag, sizeof(int));
    }

    if (status < 0)	return  RAISE_SYSERR(lib7_state, status);
    else	        return  (flag ? LIB7_true : LIB7_false);
}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
