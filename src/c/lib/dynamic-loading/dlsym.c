// dlsym.c

#include "../../mythryl-config.h"

#include <stdio.h>

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

Val   _lib7_U_Dynload_dlsym   (Task* task, Val arg)   {		// : (one_word_unt::Unt * String) -> one_word_unt::Unt
    //=====================
    //
    // Extract symbol from dynamically loaded library.

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_U_Dynload_dlsym");

    Val lib7_handle =                           GET_TUPLE_SLOT_AS_VAL (arg, 0);
    char* symname   =  HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL (arg, 1));

    void* handle    =  (void*) (WORD_LIB7toC (lib7_handle));
    void* address;


    Mythryl_Heap_Value_Buffer symname_buf;

    {	char* symname_c
	    =
	    buffer_mythryl_heap_value(  &symname_buf,  (void*) symname,  strlen(symname)+1 );	// '+1' for terminal NUL at end of string.

	#ifdef OPSYS_WIN32
	    address = GetProcAddress (handle, symname);
	    //
	    if (address == NULL && symname != NULL)	  dlerror_set ("Symbol `%s' not found", symname);
	#else
	    RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_U_Dynload_dlsym", NULL );
		//
		address = dlsym( handle, symname_c );
		//
	    RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_U_Dynload_dlsym" );
	#endif

	unbuffer_mythryl_heap_value( &symname_buf );
    }

    return  make_one_word_unt(task,  (Vunt) address  );
}


// COPYRIGHT (c) 2000 by Lucent Technologies, Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

