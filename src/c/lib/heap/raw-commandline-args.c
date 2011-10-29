// raw-commandline-args.c

#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"

// One of the library bindings exported via
//     src/c/lib/heap/cfun-list.h
// and thence
//     src/c/lib/heap/libmythryl-heap.c


Val   _lib7_proc_raw_commandline_args   (Task* task,  Val arg)   {
    //===================
    //
    // Mythryl type:  Void -> List(String)
    //
    // This fn gets bound to 'get_all_arguments' in:
    //
    //     src/lib/std/commandline.pkg
    //
    return   make_ascii_strings_from_vector_of_c_strings( task, raw_args );
}



// COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

