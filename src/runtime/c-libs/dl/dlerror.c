/* dlerror.c
 *
 */

#include "../../config.h"

#ifndef OPSYS_WIN32
# include "runtime-unixdep.h"
#endif

#if HAVE_DLFCN_H
# include <dlfcn.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#ifdef OPSYS_WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* roll-your-own dlerror... */
static int dl_error_read = 0;
static char *dl_error = NULL;

void dlerror_set (const char *fmt, const char *s)
{
  if (dl_error != NULL)
    free (dl_error);
  dl_error = malloc (strlen (fmt) + strlen (s) + 1);
  sprintf (dl_error, fmt, s);
  dl_error_read = 0;
}

char *dlerror (void)
{
  if (dl_error)
    if (dl_error_read) {
      free (dl_error);
      dl_error = NULL;
    } else
      dl_error_read = 1;

  return dl_error;
}
#endif

/* _lib7_P_Dynload_dlerror : Void -> String option
 *
 * Extract error after unsuccessful dlopen/dlsym/dlclose.
 */
lib7_val_t _lib7_U_Dynload_dlerror (lib7_state_t *lib7_state, lib7_val_t lib7_handle)
{
  const char *e = dlerror ();
  lib7_val_t r, s;

  if (e == NULL)
    r = OPTION_NONE;
  else {
    s = LIB7_CString (lib7_state, e);
    OPTION_SOME (lib7_state, r, s);
  }
  return r;
}


/* COPYRIGHT (c) 2000 by Lucent Technologies, Bell Laboratories
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
