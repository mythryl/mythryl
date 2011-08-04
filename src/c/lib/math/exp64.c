// exp64.c

#include "../../config.h"

#include <math.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"



Val   _lib7_Math_exp64   (Task* task,  Val arg)   {
    //================
    //
    double  d  =  *(PTR_CAST(double*, arg));

    Val result;
    //
    REAL64_ALLOC(task, result, exp(d));
    //
    return result;
}


// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

