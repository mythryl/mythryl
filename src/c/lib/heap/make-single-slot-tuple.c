// make-single-slot-tuple.c
//
// Create a singleton record.


/*
###               "He travels the fastest who travels alone."
###
###                                 -- Rudyard Kipling
*/


#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "task.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"


// One of the library bindings exported via
//     src/c/lib/heap/cfun-list.h
// and thence
//     src/c/lib/heap/libmythryl-heap.c



Val   _lib7_runtime_make_single_slot_tuple   (Task* task,   Val arg)   {
    //====================================
    //
    // Mythryl type:  Chunk -> Chunk
    //
    // This fn gets bound to   make_single_slot_tuple   in:
    //
    //     src/lib/std/src/unsafe/unsafe-chunk.pkg

    Val               result;
    REC_ALLOC1( task, result, arg );						// REC_ALLOC1		def in    src/c/h/make-strings-and-vectors-etc.h
    return            result;
}



// COPYRIGHT (c) 1998 Bell Labs, Lucent Technologies.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


