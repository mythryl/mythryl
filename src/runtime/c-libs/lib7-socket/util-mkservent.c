/* util-mkservent.c
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



/*
###             "Rail travel at high speeds is not possible because
###              passengers, unable to breathe, would die of asphyxia."
###
###                           -- Dionysius Lardner, 1830,
###                              Professor of Natural Philosophy and Astronomy at University College, London,
###                              author of The Steam Engine Explained and Illustrated.
###
*/



/* _util_NetDB_mkservent:
 *
 * Allocate an Lib7 value of type:
 *    (String * String list * int * String) option
 * to represent a struct servent value.  Note that the port number is returned
 * in network byteorder, so we need to map it to host order.
 */
lib7_val_t _util_NetDB_mkservent (lib7_state_t *lib7_state, struct servent *sentry)
{
    if (sentry == NULL)
	return OPTION_NONE;
    else {
      /* build the return result */
	lib7_val_t	name, aliases, port, proto, res;

	name = LIB7_CString(lib7_state, sentry->s_name);
	aliases = LIB7_CStringList(lib7_state, sentry->s_aliases);
	port = INT_CtoLib7(ntohs(sentry->s_port));
	proto = LIB7_CString(lib7_state, sentry->s_proto);
	REC_ALLOC4 (lib7_state, res, name, aliases, port, proto);
	OPTION_SOME (lib7_state, res, res);
	return res;
    }

} /* end of _util_NetDB_mkservent */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
