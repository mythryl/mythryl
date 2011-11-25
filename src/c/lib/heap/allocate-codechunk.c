// allocate-codechunk.c

#include "../../mythryl-config.h"

#include "flush-instruction-cache-system-dependent.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"


// One of the library bindings exported via
//     src/c/lib/heap/cfun-list.h
// and thence
//     src/c/lib/heap/libmythryl-heap.c

Val   _lib7_runtime_allocate_codechunk   (Task* task,  Val arg) {
    //================================
    //
    // Mythryl type:   Int -> rw_vector_of_one_byte_unts::Rw_Vector
    //
    // Allocate a code chunk of the given size-in-bytes.
    //
    // Note: Generating the name string within the code chunk
    //       is the code generator's responsibility.
    //
    // This fn gets bound to 'alloc_code' in:
    //
    //     src/lib/compiler/execution/code-segments/code-segment.pkg

    int   nbytes =   TAGGED_INT_TO_C_INT( arg );
    Val	  code   =   allocate_nonempty_code_chunk( task, nbytes );		// allocate_nonempty_code_chunk		def in    src/c/heapcleaner/make-strings-and-vectors-etc.c

    Val	               result;
    SEQHDR_ALLOC(task, result, UNT8_RW_VECTOR_TAGWORD, code, nbytes);
    return             result;
}


// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

