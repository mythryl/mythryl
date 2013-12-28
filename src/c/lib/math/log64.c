// log64.c

#include "../../mythryl-config.h"

#include <math.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"



Val   _lib7_Math_log64   (Task* task,   Val arg)   {
    //================
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
    double d =  *(PTR_CAST(double*, arg));

    Val result =  make_float64(task, log(d) );
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

