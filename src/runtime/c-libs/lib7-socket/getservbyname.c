/* getservbyname.c
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
###           "I see little commercial potential for
###            the Internet for at least ten years."
###
###                         -- Bill Gates, 1994
 */


/* _lib7_NetDB_getservbyname
 *     : (String * String option) -> (String * String list * int * String) option
 */
lib7_val_t _lib7_NetDB_getservbyname (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	mlServ = REC_SEL(arg, 0);
    lib7_val_t	mlProto = REC_SEL(arg, 1);
    char	*proto;

    if (mlProto == OPTION_NONE)
	proto = NULL;
    else
	proto = STR_LIB7toC(OPTION_get(mlProto));

    return _util_NetDB_mkservent (
	lib7_state,
	getservbyname (STR_LIB7toC(mlServ), proto));

} /* end of _lib7_NetDB_getservbyname */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
