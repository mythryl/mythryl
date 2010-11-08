/* win32-errors.c
 *
 * interface to win32 error functions
 */

#include "../../config.h"

#include <windows.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"

/* _lib7_win32_get_last_error: Void -> word
 */
lib7_val_t _lib7_win32_get_last_error(lib7_state_t *lib7_state, lib7_val_t arg)
{
    Word_t	err = (Word_t)GetLastError();
    lib7_val_t	res;

    WORD_ALLOC(lib7_state, res, err);

    return res;
}

/* end of win32-errors.c */



/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

