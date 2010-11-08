/* win32-util.c
 *
 * win32 specific utility code
 */

#include "../config.h"

#include <windows.h>
#include "runtime-osdep.h"
#include "runtime-base.h"

int GetPageSize()
{
  SYSTEM_INFO si;

  GetSystemInfo(&si);
  return (int) si.dwPageSize;
}

/* end of win32-util.c */


/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

