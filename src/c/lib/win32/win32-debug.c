/* win32-debug.c
 *
 * win32 debug support
 */

#include "../../mythryl-config.h"

#include <windows.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"

/* _lib7_win32_debug: String -> word
 */
Val _lib7_win32_debug(Task *task, Val arg)
{
  printf("%s",arg);	fflush(stdout);
  return HEAP_VOID;
}

/* end of win32-debug.c */



/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
 * released under Gnu Public Licence version 3.
 */

