/* win32-errors.c
 *
 * interface to win32 error functions
 */

#include "../../mythryl-config.h"

#include <windows.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"

/* _lib7_win32_get_last_error: Void -> word
 */
Val _lib7_win32_get_last_error(Task *task, Val arg)
{
    Val_Sized_Unt	err = (Val_Sized_Unt)GetLastError();

    return make_one_word_unt(task,  err  );
}

/* end of win32-errors.c */



/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
 * released under Gnu Public Licence version 3.
 */

