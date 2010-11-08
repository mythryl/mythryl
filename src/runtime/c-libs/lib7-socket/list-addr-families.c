/* list-addr-families.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-heap.h"
#include "socket-util.h"
#include "cfun-proto-list.h"
#include "lib7-c.h"



/*
###             "Atomic energy might be as good as our present-day explosives,
###              but it is unlikely to produce anything very much more dangerous."
###
###                                -- Winston Churchill, 1939
*/



/* _lib7_Sock_listaddrfamilies:
 *
 * Return a list of the known address famlies (this may contain unsupported
 * families).
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t _lib7_Sock_listaddrfamilies (lib7_state_t *lib7_state, lib7_val_t arg)
{
    return LIB7_SysConstList (lib7_state, &_Sock_AddrFamily);

} /* end of _lib7_Sock_listaddrfamilies */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
