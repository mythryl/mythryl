// alarm.c



#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
    #include <unistd.h>
#endif



Val   _lib7_P_Process_alarm   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type:   Int -> Int
    //
    // Set a process alarm clock
    //
    return TAGGED_INT_FROM_C_INT( alarm( TAGGED_INT_TO_C_INT( arg )));
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

