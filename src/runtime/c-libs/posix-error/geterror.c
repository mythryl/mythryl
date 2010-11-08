/* geterror.c
 *
 * Return the system constant that corresponds to the given error name.
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

extern sysconst_table_t	_ErrorNo;


/* _lib7_P_Error_geterror : int -> sys_const
 */
lib7_val_t _lib7_P_Error_geterror (lib7_state_t *lib7_state, lib7_val_t arg)
{
    return LIB7_SysConst (lib7_state, &_ErrorNo, INT_LIB7toC(arg));

} /* end of _lib7_P_Error_geterror */


/* COPYRIGHT (c) 1996 AT&T Research.
 *
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
