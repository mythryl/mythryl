// dlerror.c

#include "../../mythryl-config.h"

#ifndef OPSYS_WIN32
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

#ifdef OPSYS_WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Roll-your-own dlerror...
//
static int dl_error_read = 0;
static char *dl_error = NULL;

void   dlerror_set   (const char *fmt, const char *s) {
    // ===========
    //
    if (dl_error != NULL)   free (dl_error);

    dl_error = malloc (strlen (fmt) + strlen (s) + 1);
    sprintf (dl_error, fmt, s);
    dl_error_read = 0;
}

char*   dlerror   (void)   {
    //  =======
    //
    if (dl_error) {
	if (!dl_error_read) {
	  dl_error_read = 1;
	} else {
	  free (dl_error);
	  dl_error = NULL;
	}
    }
    return dl_error;
}
#endif

Val   _lib7_U_Dynload_dlerror   (Task* task, Val lib7_handle)   { 	// : Void -> Null_Or(String)
    //=======================
    //
    // Extract error after unsuccessful dlopen/dlsym/dlclose.

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_U_Dynload_dlerror");
    const char* e =  dlerror ();

    if (e == NULL)    return  OPTION_NULL;
    else 	      return  OPTION_THE(  task,  make_ascii_string_from_c_string__may_heapclean(task, e)  );
}


// COPYRIGHT (c) 2000 by Lucent Technologies, Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

