/* win32-errors.c
 *
 * interface to win32 error functions
 */

#include "../../mythryl-config.h"

#include <windows.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"

/* _lib7_win32_get_last_error: Void -> word
 */
Val _lib7_win32_get_last_error(Task *task, Val arg)
{
    Vunt	err = (Vunt)GetLastError();

    return make_one_word_unt(task,  err  );
}

/* end of win32-errors.c */



/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
 * released under Gnu Public Licence version 3.
 */

