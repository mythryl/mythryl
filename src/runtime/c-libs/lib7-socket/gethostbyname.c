/* gethostbyname.c
 *
 */

#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "socket-util.h"

/*
###             "[It would not be long] ere the whole surface
###              of this country would be channelled for those
###              nerves which are to diffuse, with the speed
###              of thought, a knowledge of all that is occurring
###              throughout the land, making, in fact,
###              one neighborhood of the whole country."
###
###                                  -- Samuel Morse, 1838
 */

/* _lib7_NetDB_gethostbyname
 *     : String -> Null_Or (String, List( String ), Address_Family, List( Address))
 */
lib7_val_t

_lib7_NetDB_gethostbyname (lib7_state_t *lib7_state, lib7_val_t arg)
{
    return _util_NetDB_mkhostent (lib7_state, gethostbyname (STR_LIB7toC(arg)));
}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
