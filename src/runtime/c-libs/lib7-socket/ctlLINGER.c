/* ctlLINGER.c
 *
 */

#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "socket-util.h"
#include "cfun-proto-list.h"

/*
###        "We hope the Professor from Clark College
###         (Robert H. Goddard) is only pretending
###         to be ignorant of elementary physics
###         if he thinks that a rocket can work in a vacuum."
###
###                 -- Editorial, The New York Times 1920
 */

/* _lib7_Sock_ctlLINGER : (Socket, Null_Or( Null_Or( Int ))) -> int option
 *
 * Set/get the SO_LINGER option as follows:
 *   NULL		=> get current setting
 *   THE(NULL)		=> disable linger
 *   THE(THE t)	=> enable linger with timeout t.
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t

_lib7_Sock_ctlLINGER (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		    socket = REC_SELINT(arg, 0);
    lib7_val_t	    ctl = REC_SEL(arg, 1);
    struct linger   optVal;
    int		    status;

    if (ctl == OPTION_NONE) {
	int	optSz = sizeof(struct linger);
	status = getsockopt (socket, SOL_SOCKET, SO_LINGER, (sockoptval_t)&optVal, &optSz);
	ASSERT((status < 0) || (optSz == sizeof(struct linger)));
    }
    else {
	ctl = OPTION_get(ctl);
	if (ctl == OPTION_NONE) {
	  /* argument is THE(NULL); disable linger */
	    optVal.l_onoff = 0;
	}
	else {
	  /* argument is THE t; enable linger */
	    optVal.l_onoff = 1;
	    optVal.l_linger = INT_LIB7toC(OPTION_get(ctl));
	}
	status = setsockopt (socket, SOL_SOCKET, SO_LINGER, (sockoptval_t)&optVal, sizeof(struct linger));
    }

    if (status < 0)
        return RAISE_SYSERR(lib7_state, status);
    else if (optVal.l_onoff == 0)
	return OPTION_NONE;
    else {
	lib7_val_t	res;
	OPTION_SOME(lib7_state, res, INT_CtoLib7(optVal.l_linger));
	return res;
    }

} /* end of _lib7_Sock_ctlLINGER */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
