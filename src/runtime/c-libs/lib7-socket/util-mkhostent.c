/* util-mkhostent.c
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
###                        "Louis Pasteur's theory of germs is ridiculous fiction."
###                                 -- Pierre Pachet, 1872,
###                                    professor of physiology at Toulouse
*/



/* _util_NetDB_mkhostent:
 *
 * Allocate an Lib7 value of type
 *    (String * String list * addr_family * addr list) option
 * to represent a struct hostent value.
 *
 * NOTE: we should probably be passing back the value of h_errno, but this
 * will require an API change at the Lib7 level.  XXX BUGGO FIXME
 */
lib7_val_t _util_NetDB_mkhostent (lib7_state_t *lib7_state, struct hostent *hentry)
{
    if (hentry == NULL)
	return OPTION_NONE;
    else {
      /* build the return result */
	lib7_val_t	name, aliases, af, addr, addresses, res;
	int		nAddresses, i;

	name = LIB7_CString(lib7_state, hentry->h_name);
	aliases = LIB7_CStringList(lib7_state, hentry->h_aliases);
	af = LIB7_SysConst (lib7_state, &_Sock_AddrFamily, hentry->h_addrtype);
	for (nAddresses = 0;  hentry->h_addr_list[nAddresses] != NULL;  nAddresses++)
	    continue;
	for (i = nAddresses, addresses = LIST_nil;  --i >= 0;  ) {
	    addr = LIB7_AllocString (lib7_state, hentry->h_length);
	    memcpy (GET_SEQ_DATAPTR(void, addr), hentry->h_addr_list[i],
		hentry->h_length);
	    LIST_cons(lib7_state, addresses, addr, addresses);
	}
	REC_ALLOC4 (lib7_state, res, name, aliases, af, addresses);
	OPTION_SOME (lib7_state, res, res);
	return res;
    }

} /* end of _util_NetDB_mkhostent */



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
