// make-codechunk-executable.c

#include "../../mythryl-config.h"

#include "flush-instruction-cache-system-dependent.h"
#include "runtime-base.h"
#include "make-strings-and-vectors-etc.h"
#include "task.h"
#include "cfun-proto-list.h"


// One of the library bindings exported via
//     src/c/lib/heap/cfun-list.h
// and thence
//     src/c/lib/heap/libmythryl-heap.c



Val   _lib7_runtime_make_codechunk_executable   (Task* task,  Val arg)   {
    //=======================================
    //
    // Mythryl type:  (rw_vector_of_one_byte_unts::Rw_Vector, Int) -> (Chunk -> Chunk)	// The Int is the entrypoint offset within the bytevector of executable machine code -- currently always zero in practice.
    //
    // Turn a previously constructed machine-code bytvector into a closure.
    // This requires that we flush the I-cache. (This is a no-op on intel32.)
    //
    // This fn gets bound as   make_executable   in:
    //
    //     src/lib/compiler/execution/code-segments/code-segment.pkg


    Val   seq        =  GET_TUPLE_SLOT_AS_VAL( arg, 0 );
    int   entrypoint =  GET_TUPLE_SLOT_AS_INT( arg, 1 );			// In practice entrypoint is currently always zero.

    char* code =  GET_VECTOR_DATACHUNK_AS( char*, seq );

    Val_Sized_Unt nbytes		/* This variable is unused on some platforms, so suppress 'unused var' compiler warning: */   __attribute__((unused))
        =
        GET_VECTOR_LENGTH( seq );

    flush_instruction_cache( code, nbytes );					// flush_instruction_cache is a no-op on intel32
										// flush_instruction_cache	def in    src/c/h/flush-instruction-cache-system-dependent.h 
    Val	             result;
    REC_ALLOC1(task, result, PTR_CAST( Val, code + entrypoint));
    return           result;
}



// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

