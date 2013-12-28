// dlopen.c

#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

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
#include "raise-error.h"
#include "cfun-proto-list.h"

Val   _lib7_U_Dynload_dlopen   (Task* task, Val arg)   {	//  (String, Bool, Bool) -> one_word_unt::Unt
    //======================
    //
    // Open a dynamically loaded library.
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Val ml_libname = GET_TUPLE_SLOT_AS_VAL (arg, 0);
    int lazy       = GET_TUPLE_SLOT_AS_VAL (arg, 1) == HEAP_TRUE;
    int global     = GET_TUPLE_SLOT_AS_VAL (arg, 2) == HEAP_TRUE;

    char *libname = NULL;
    void *handle;


    Mythryl_Heap_Value_Buffer  libname_buf;

    if (ml_libname != OPTION_NULL) {
	//
        libname = HEAP_STRING_AS_C_STRING (OPTION_GET (ml_libname));
	//
	// Copy libname out of Mythryl heap to
        // make it safe to reference between
	// RELEASE_MYTHRYL_HEAP and
	// RECOVER_MYTHRYL_HEAP:
	//
	libname =  (char*)  buffer_mythryl_heap_value( &libname_buf, (void*)libname, strlen(libname)+1 );		// '+1' for terminal NUL on string.
    }

    #ifdef OPSYS_WIN32

	handle = (void *) LoadLibrary (libname);

	if (handle == NULL && libname != NULL)	  dlerror_set ("Library `%s' not found", libname);

    #else
	int flag = (lazy ? RTLD_LAZY : RTLD_NOW);

	if (global) flag |= RTLD_GLOBAL;

	RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	    //
	    handle = dlopen (libname, flag);
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );
    #endif

    if (libname)  unbuffer_mythryl_heap_value( &libname_buf );

    Val result =  make_one_word_unt(task,  (Vunt) handle  );
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 2000 by Lucent Technologies, Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

