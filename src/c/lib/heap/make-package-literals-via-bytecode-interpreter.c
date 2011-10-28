// make-package-literals-via-bytecode-interpreter.c

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



Val   _lib7_runtime_make_package_literals_via_bytecode_interpreter   (Task* task,  Val arg)   {
    //============================================================
    //
    // Mythryl type:   vector_of_one_byte_unts::Vector -> Vector(Chunk)
    //
    // This fn gets bound as
    //
    //      make_package_literals_via_bytecode_interpreter
    // in
    //     src/lib/compiler/execution/code-segments/code-segment.pkg
    //
    // and ultimately invoked in
    //
    //     src/lib/compiler/execution/main/execute.pkg
    //
    return   make_package_literals_via_bytecode_interpreter (						// make_package_literals_via_bytecode_interpreter	def in    src/c/heapcleaner/make-package-literals-via-bytecode-interpreter.c
                 task,
                 GET_VECTOR_DATACHUNK_AS( Unt8*, arg ),
                 GET_VECTOR_LENGTH( arg )
             );
}


// COPYRIGHT (c) 1998 Bell Labs, Lucent Technologies.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

