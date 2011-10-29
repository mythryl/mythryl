// dummy.c
//
// This is a dummy run-time routine for when
// we would like to call a null C function.


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "cfun-proto-list.h"

Val   _lib7_runtime_dummy   (Task* task,  Val arg)   {
    //===================
    // 
    // Mythryl type:  String -> Void
    //
    // The string argument can be used as a unique marker.
    //

    /*
      char	*s = HEAP_STRING_AS_C_STRING(arg);
    */

    return HEAP_VOID;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


