// datastructure-pickler.c

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "heap-io.h"
#include "cfun-proto-list.h"

// One of the library bindings exported via
//     src/c/lib/heap/cfun-list.h
// and thence
//     src/c/lib/heap/libmythryl-heap.c


// For background see:  src/A.DATASTRUCTURE-PICKLING.OVERVIEW

Val   _lib7_runtime_pickle_datastructure   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:  X -> unt8_vector::Vector
    //
    // Translate a heap chunk into a linear representation (vector of bytes).
    //
    // This fn gets bound to 'pickle_datastructure' in:
    //
    //     src/lib/std/src/unsafe/unsafe.pkg

    Val  pickle =  pickle_datastructure( task, arg );								// pickle_datastructure	def in   src/c/cleaner/datastructure-pickler.c

    if (pickle == HEAP_VOID)   return RAISE_ERROR(task, "Attempt to pickle datastructure failed");		// XXX BUGGO FIXME Need a clearer diagnostic here.
    else                       return pickle;
}



// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

