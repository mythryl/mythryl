/* win32-debug.c
 *
 * win32 debug support
 */

#include "../../config.h"

#include <windows.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"

/* _lib7_win32_debug: String -> word
 */
lib7_val_t _lib7_win32_debug(lib7_state_t *lib7_state, lib7_val_t arg)
{
  printf("%s",arg);
  return LIB7_void;
}

/* end of win32-debug.c */



/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

