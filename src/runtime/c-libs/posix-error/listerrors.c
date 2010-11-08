/* listerrors.c
 *
 * Return the list of system constants that represents the known error
 * codes.
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

extern sysconst_table_t	_ErrorNo;						/* See  src/runtime/c-libs/posix-error/tbl-errno.c	*/


/* _lib7_P_Error_listerrors : int -> sys_const list
 */
lib7_val_t   _lib7_P_Error_listerrors   (lib7_state_t *lib7_state, lib7_val_t arg)
{
    return LIB7_SysConstList (lib7_state, &_ErrorNo);				/* See  src/runtime/gc/runtime-heap.c			*/
}


/* COPYRIGHT (c) 1996 AT&T Research.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
