// program-name-from-commandline.c

#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"

// One of the library bindings exported via
//     src/c/lib/heap/cfun-list.h
// and thence
//     src/c/lib/heap/libmythryl-heap.c


Val   _lib7_proc_program_name_from_commandline   (Task* task,  Val arg)   {
    //===================
    //
    // Mythryl type:   Void -> String
    //
    // This fn gets bound to 'get_program_name' in:
    //
    //     src/lib/std/commandline.pkg

    return   make_ascii_string_from_c_string( task, mythryl_program_name__global );
}



// COPYRIGHT (c) 1997 Bell Labs, Lucent Technologies.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

