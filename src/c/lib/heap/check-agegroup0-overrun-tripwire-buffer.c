// check-agegroup0-overrun-tripwire-buffer.c
//
// General interface for heapcleaner control functions.
//

#include "../../mythryl-config.h"

#include <string.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "get-multipage-ram-region-from-os.h"
#include "heap.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/heap/cfun-list.h
// and thence
//     src/c/lib/heap/libmythryl-heap.c



Val   _lib7_check_agegroup0_overrun_tripwire_buffer  (Task* task,  Val arg)   {
    //=============================================
    //
    // Mythryl type:   String -> Void
    //
    char* caller = HEAP_STRING_AS_C_STRING(arg);
    //
    check_agegroup0_overrun_tripwire_buffer( task, caller );
    //
    return HEAP_VOID;
}

// By Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


