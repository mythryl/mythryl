// concatenate-two-tuples.c
//
// Concatenation for records.


#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "task.h"
#include "make-strings-and-vectors-etc.h"
#include "heapcleaner.h"
#include "cfun-proto-list.h"
#include "lib7-c.h"



// One of the library bindings exported via
//     src/c/lib/heap/cfun-list.h
// and thence
//     src/c/lib/heap/libmythryl-heap.c



Val   _lib7_runtime_concatenate_two_tuples   (Task* task,  Val arg)   {
    //====================================
    //
    // Mythryl type:   (Chunk, Chunk) -> Chunk
    //
    // This fn gets bound as   r_meld   in:
    //
    //     src/lib/std/src/unsafe/unsafe-chunk.pkg

    Val    r1 = GET_TUPLE_SLOT_AS_VAL(arg,0);
    Val    r2 = GET_TUPLE_SLOT_AS_VAL(arg,1);

    if (r1 == HEAP_VOID)	return r2;
    else if (r2 == HEAP_VOID)	return r1;
    else {
      Val  result =   concatenate_two_tuples (task, r1, r2);					// concatenate_two_tuples	def in   src/c/heapcleaner/tuple-ops.c

	if (result == HEAP_VOID)   return RAISE_ERROR( task, "recordmeld: not a record");
	else                       return result;
    }
}



// COPYRIGHT (c) 1998 Bell Labs, Lucent Technologies.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


