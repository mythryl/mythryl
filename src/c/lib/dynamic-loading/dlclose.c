// dlclose.c

#include "../../config.h"

#ifdef OPSYS_WIN32
# include <windows.h>
#else
# include "system-dependent-unix-stuff.h"
#endif

#if HAVE_DLFCN_H
# include <dlfcn.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

Val   _lib7_U_Dynload_dlclose   (Task* task, Val lib7_handle)   {	// : one_word_unt::Unt -> Void
    //=======================
    //
    // Close dynamically loaded library.
    //
    void* handle = (void*) (WORD_LIB7toC (lib7_handle));

    #ifdef OPSYS_WIN32
      (void) FreeLibrary (handle);
    #else
      (void) dlclose (handle);
    #endif
  
    return HEAP_VOID;
}


// COPYRIGHT (c) 2000 by Lucent Technologies, Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

