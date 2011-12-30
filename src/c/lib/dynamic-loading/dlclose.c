// dlclose.c

#include "../../mythryl-config.h"

#include <stdio.h>

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

    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_U_Dynload_dlclose");

    void* handle = (void*) (WORD_LIB7toC (lib7_handle));

    #ifdef OPSYS_WIN32
      (void) FreeLibrary (handle);
    #else
	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_U_Dynload_dlclose", lib7_handle );
	    //
	    (void) dlclose (handle);							// BN: 'handle' should not be pointing into the Mythryl heap (check dlopen.c) so it should be safe to use between RELEASE/RECOVER.
	    //
	    // Should likely check return value here!  XXX SUCKO FIXME
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_U_Dynload_dlclose" );
    #endif
  
    return HEAP_VOID;
}


// COPYRIGHT (c) 2000 by Lucent Technologies, Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

