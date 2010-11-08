/* dlopen.c
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

/* _lib7_P_Dynload_dlopen : String * Bool * Bool -> unt32.word
 *
 * Open a dynamically loaded library.
 */
lib7_val_t _lib7_U_Dynload_dlopen (lib7_state_t *lib7_state, lib7_val_t arg)
{
  lib7_val_t ml_libname = REC_SEL (arg, 0);
  int lazy = REC_SEL (arg, 1) == LIB7_true;
  int global = REC_SEL (arg, 2) == LIB7_true;
  char *libname = NULL;
  void *handle;
  lib7_val_t res;

  if (ml_libname != OPTION_NONE)
    libname = STR_LIB7toC (OPTION_get (ml_libname));

#ifdef OPSYS_WIN32

  handle = (void *) LoadLibrary (libname);
  if (handle == NULL && libname != NULL)
    dlerror_set ("Library `%s' not found", libname);

#else
  {
    int flag = (lazy ? RTLD_LAZY : RTLD_NOW);
    
    if (global) flag |= RTLD_GLOBAL;

    handle = dlopen (libname, flag);
  }
#endif
  
  WORD_ALLOC (lib7_state, res, (Word_t) handle);
  return res;
}


/* COPYRIGHT (c) 2000 by Lucent Technologies, Bell Laboratories
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
