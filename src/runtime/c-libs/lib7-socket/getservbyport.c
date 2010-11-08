/* getservbyport.c
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
###                     "The energy produced by the atom is a very poor kind of thing.
###                      Anyone who expects a source of power from the transformation
###                      of these atoms is talking moonshine."
###
###                                                -- Ernst Rutherford, 1933
*/



/* _lib7_NetDB_getservbyport
 *     : (int * String option) -> (String * String list * int * String) option
 */
lib7_val_t _lib7_NetDB_getservbyport (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	mlProto = REC_SEL(arg, 1);
    char	*proto;

    if (mlProto == OPTION_NONE)
	proto = NULL;
    else
	proto = STR_LIB7toC(OPTION_get(mlProto));

    return _util_NetDB_mkservent (lib7_state, getservbyport (REC_SELINT(arg, 0), proto));

} /* end of _lib7_NetDB_getservbyport */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
