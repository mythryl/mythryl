/* dlsym.c
 *
 */

#include "../../config.h"

#ifdef OPSYS_WIN32
# include <windows.h>
extern void dlerror_set (const char *fmt, const char *s);
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

/* _lib7_P_Dynload_dlsym : unt32.word * String -> unt32.word
 *
 * Extract symbol from dynamically loaded library.
 */
lib7_val_t _lib7_U_Dynload_dlsym (lib7_state_t *lib7_state, lib7_val_t arg)
{
  lib7_val_t lib7_handle = REC_SEL (arg, 0);
  char *symname = STR_LIB7toC (REC_SEL (arg, 1));
  void *handle = (void *) (WORD_LIB7toC (lib7_handle));
  void *addr;
  lib7_val_t res;

#ifdef OPSYS_WIN32
  addr = GetProcAddress (handle, symname);
  if (addr == NULL && symname != NULL)
    dlerror_set ("Symbol `%s' not found", symname);
#else
  addr = dlsym (handle, symname);
#endif
  
  WORD_ALLOC (lib7_state, res, (Word_t) addr);
  return res;
}


/* COPYRIGHT (c) 2000 by Lucent Technologies, Bell Laboratories
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
