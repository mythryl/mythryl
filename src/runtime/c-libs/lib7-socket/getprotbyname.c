/* getprotbyname.c
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
###                     "It is now possible for a business man to
###                      talk with his office from a moving vehicle.
###                      The apparatus necessary to do this marvellous
###                      thing can be carried in a small dress suit case."
###
###                                          -- John Brady, 1920
 */


/* _lib7_NetDB_getprotbyname : String -> (String * String list * int) option
 */
lib7_val_t _lib7_NetDB_getprotbyname (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	    name, aliases, res;
    struct protoent *pentry;

    pentry = getprotobyname (STR_LIB7toC(arg));

    if (pentry == NULL)
	return OPTION_NONE;
    else {
	name = LIB7_CString (lib7_state, pentry->p_name);
	aliases = LIB7_CStringList (lib7_state, pentry->p_aliases);
	REC_ALLOC3 (lib7_state, res, name, aliases, INT_CtoLib7(pentry->p_proto));
	OPTION_SOME (lib7_state, res, res);
	return res;
    }

} /* end of _lib7_NetDB_getprotbyname */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
