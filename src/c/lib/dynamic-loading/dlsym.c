// dlsym.c

#include "../../config.h"

#ifdef OPSYS_WIN32
# include <windows.h>
extern void dlerror_set (const char *fmt, const char *s);
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

Val   _lib7_U_Dynload_dlsym   (Task* task, Val arg)   {		// : (unt32::Unt * String) -> unt32::Unt
    //=====================
    //
    // Extract symbol from dynamically loaded library.

    Val lib7_handle =  GET_TUPLE_SLOT_AS_VAL (arg, 0);
    char* symname   =  HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL (arg, 1));
    void* handle    =  (void*) (WORD_LIB7toC (lib7_handle));
    void* address;

    Val result;

    #ifdef OPSYS_WIN32
	address = GetProcAddress (handle, symname);
	//
	if (address == NULL && symname != NULL)	  dlerror_set ("Symbol `%s' not found", symname);
    #else
	address = dlsym (handle, symname);
    #endif

    WORD_ALLOC (task, result, (Val_Sized_Unt) address);

    return result;
}


// COPYRIGHT (c) 2000 by Lucent Technologies, Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

