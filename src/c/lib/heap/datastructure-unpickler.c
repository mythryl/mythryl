// datastructure-unpickler.c

#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "heap-io.h"
#include "cfun-proto-list.h"

// For background see:  src/A.DATASTRUCTURE-PICKLING.OVERVIEW

// One of the library bindings exported via
//     src/c/lib/heap/cfun-list.h
// and thence
//     src/c/lib/heap/libmythryl-heap.c

Val   _lib7_runtime_unpickle_datastructure   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:   String -> X
    //
    // Build a Mythryl value from a string.
    //
    // This fn gets bound to 'unpickle_datastructure' in:
    //
    //     src/lib/std/src/unsafe/unsafe.pkg

    Bool	seen_error = FALSE;

    Val datastructure
	=
	unpickle_datastructure(				// unpickle_datastructure	def in    src/c/heapcleaner/datastructure-unpickler.c
	    //
	    task,
	    PTR_CAST(Unt8*, arg),
	    CHUNK_LENGTH(arg),
	    &seen_error
	);

    if (seen_error)  	return RAISE_ERROR( task, "unpickle_datastructure");
    else         	return datastructure;
}



// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

