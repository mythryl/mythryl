// release-pthread.c

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-pthread.h"
#include "cfun-proto-list.h"

// One of the library bindings exported via
//     src/c/lib/pthread/cfun-list.h
// and thence
//     src/c/lib/pthread/libmythryl-pthread.c 


Val   _lib7_MP_release_pthread   (Task* task,  Val arg)   {
    //=====================
    //
    #ifdef MULTICORE_SUPPORT
	pth_release_pthread(task);  	// Should not return.
	die ("_lib7_MP_release_pthread: call unexpectedly returned\n");
    #else
	die ("_lib7_MP_release_pthread: no mp support\n");
    #endif
}


// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

