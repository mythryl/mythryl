/* dlclose.c
 *
 */

#include "../../config.h"

#ifdef OPSYS_WIN32
# include <windows.h>
#else
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

/* _lib7_P_Dynload_dlclose : unt32.word -> Void
 *
 * Close dynamically loaded library.
 */
lib7_val_t _lib7_U_Dynload_dlclose (lib7_state_t *lib7_state, lib7_val_t lib7_handle)
{
  void *handle = (void *) (WORD_LIB7toC (lib7_handle));

#ifdef OPSYS_WIN32
  (void) FreeLibrary (handle);
#else
  (void) dlclose (handle);
#endif
  
  return LIB7_void;
}


/* COPYRIGHT (c) 2000 by Lucent Technologies, Bell Laboratories
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
