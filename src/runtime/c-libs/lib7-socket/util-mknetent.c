/* util-mknetent.c
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

/* _util_NetDB_mknetent:
 *
 * Allocate an Lib7 value of type
 *    (String * String list * addr_family * sysword) option
 * to represent a struct netent value.
 */
lib7_val_t _util_NetDB_mknetent (lib7_state_t *lib7_state, struct netent *nentry)
{
    if (nentry == NULL)
	return OPTION_NONE;
    else {
      /* build the return result */
	lib7_val_t	name, aliases, af, net, res;

	name = LIB7_CString(lib7_state, nentry->n_name);
	aliases = LIB7_CStringList(lib7_state, nentry->n_aliases);
	af = LIB7_SysConst (lib7_state, &_Sock_AddrFamily, nentry->n_addrtype);
	WORD_ALLOC(lib7_state, net, (Word_t)(nentry->n_net));
	REC_ALLOC4 (lib7_state, res, name, aliases, af, net);
	OPTION_SOME (lib7_state, res, res);
	return res;
    }
} /* end of _util_NetDB_mknetent */



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
