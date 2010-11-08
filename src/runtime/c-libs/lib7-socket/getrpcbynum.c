/* getrpcbynum.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"
#include "sockets-osdep.h"
#include <netdb.h>
#ifdef INCLUDE_RPCENT_H
#  include INCLUDE_RPCENT_H
#  ifdef bool_t		/* NetBSD hack */
#    undef bool_t
#  endif
#endif
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "socket-util.h"



/*
###               "While theoretically television may be feasible,
###                commercially and financially I consider it an
###                impossibility, a development of which we need
###                waste little time dreaming."
###
###                                 -- Lee de Forest, 1926
*/



/* _lib7_NetDB_getrpcbynum : int -> (String * String list * int) option
 */
lib7_val_t _lib7_NetDB_getrpcbynum (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	    name, aliases, res;
    struct rpcent   *rentry;

    rentry = getrpcbynumber (INT_LIB7toC(arg));

    if (rentry == NULL)
	return OPTION_NONE;
    else {
	name = LIB7_CString (lib7_state, rentry->r_name);
	aliases = LIB7_CStringList (lib7_state, rentry->r_aliases);
	REC_ALLOC3 (lib7_state, res, name, aliases, INT_CtoLib7(rentry->r_number));
	OPTION_SOME (lib7_state, res, res);
	return res;
    }

} /* end of _lib7_NetDB_getrpcbynum */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
