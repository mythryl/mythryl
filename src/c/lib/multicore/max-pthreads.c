// max-pthreads.c

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-multicore.h"
#include "cfun-proto-list.h"

// One of the library bindings exported via
//     src/c/lib/multicore/cfun-list.h
// and thence
//     src/c/lib/multicore/libmythryl-multicore.c 


Val   _lib7_MP_max_pthreads   (Task* task,  Val arg)   {			// Apparently nowhere invoked.
    //==================
    //
    #ifdef MULTICORE_SUPPORT
	return INT31_FROM_C_INT(mc_max_pthreads ());
    #else
	die ("_lib7_MP_max_pthreads: no mp support\n");
    #endif
}



// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

