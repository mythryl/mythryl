// spin-lock.c


#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "task.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-pthread.h"
#include "cfun-proto-list.h"

// One of the library bindings exported via
//     src/c/lib/pthread/cfun-list.h
// and thence
//     src/c/lib/pthread/libmythryl-pthread.c 

Val   _lib7_MP_spin_lock   (Task* task,  Val arg)   {
    //==================
    //
    #ifdef MULTICORE_SUPPORT
	// "This code is for use the assembly (MIPS.prim.asm) try_lock and lock"
        //         --- Original SML/NJ comment.
        //
        // 2010-11-30 CrT:
        //     'try_lock' is defined in
        //                     src/c/machine-dependent/prim.intel32.asm
	//                     src/c/machine-dependent/prim.sparc32.asm
        //                     src/c/machine-dependent/prim.pwrpc32.asm
        //                     src/c/machine-dependent/prim.intel32.masm 
        //     but function makes no obvious use of them.
        //     'try_lock is also published in RunVec in
        //                     src/c/main/construct-runtime-package.c
        //     -- possibly that route replaced this one, which bitrotted?
	Val             result;
	REF_ALLOC(task, result, HEAP_FALSE);						// REF_ALLOC	def in    src/c/h/make-strings-and-vectors-etc.h
	return          result;
    #else
	die ("lib7_spin_lock: no mp support\n");
    #endif
}


// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

