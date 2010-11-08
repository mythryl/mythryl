/* getprotbynum.c
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
###               "There is not the slightest indication that nuclear energy
###                will ever be obtainable. It would mean the atom would have
###                to be shattered at will."
###
###                                    -- Albert Einstein, 1932
*/


/* _lib7_NetDB_getprotbynum : Int -> Null_Or (String, List( String ), Int)
 */
lib7_val_t

_lib7_NetDB_getprotbynum (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	    name, aliases, res;
    struct protoent *pentry;

    pentry = getprotobynumber (INT_LIB7toC(arg));

    if (pentry == NULL)
	return OPTION_NONE;
    else {
	name = LIB7_CString (lib7_state, pentry->p_name);
	aliases = LIB7_CStringList (lib7_state, pentry->p_aliases);
	REC_ALLOC3 (lib7_state, res, name, aliases, INT_CtoLib7(pentry->p_proto));
	OPTION_SOME (lib7_state, res, res);
	return res;
    }

} /* end of _lib7_NetDB_getprotbynum */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
