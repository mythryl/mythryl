// acquire-pthread.c

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-pthread.h"
#include "cfun-proto-list.h"

// One of the library bindings exported via
//     src/c/lib/pthread/cfun-list.h
// and thence
//     src/c/lib/pthread/libmythryl-pthread.c 


Val   _lib7_MP_acquire_pthread   (Task* task,  Val arg)   {			// Apparently never called.
    //========================
    //
    #ifdef MULTICORE_SUPPORT
	//
	return mc_acquire_pthread( task, arg );					// mc_acquire_pthread	def in    src/c/pthread/pthread-on-posix-threads.c
        //									// mc_acquire_pthread	def in    src/c/pthread/pthread-on-sgi.c
    #else									// mc_acquire_pthread	def in    src/c/pthread/pthread-on-solaris.c
	die ("lib7_acquire_pthread: no mp support\n");
    #endif
}



// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

