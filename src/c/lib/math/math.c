// math.c

#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "make-strings-and-vectors-etc.h"
#include "ml-fp.h"

#include <math.h>

#define REAL_ALLOC(task, r, d)	{			\
	Task	*__task = (task);			\
	Val        *__p = __task->heap_allocation_pointer;	\
	double          *__dp;                          \
	*__p++ = FLOAT64_TAGWORD;				\
	__dp = (double *) __p;                          \
	*__dp++ = (d);          			\
	(r) = PTR_CAST( Val, __task->heap_allocation_pointer + 1);	\
	__task->heap_allocation_pointer = (Val *) __dp;		\
    }

Val c_cos(Task *task, Val arg)
{
    double d;
    Val result;

    Save_LIB7_FPState();
    Restore_C_FPState();
    d = cos(*(PTR_CAST(double*,arg)));
    REAL_ALLOC(task,result,d);
    Restore_LIB7_FPState();
    return result;
}

Val c_sin(Task *task, Val arg)
{
    double d;
    Val result;

    Save_LIB7_FPState();
    Restore_C_FPState();
    d = sin(*(PTR_CAST(double*,arg)));
    REAL_ALLOC(task,result,d);
    Restore_LIB7_FPState();
    return result;
}

Val c_exp(Task *task, Val arg)
{
    double d;
    Val result;
    extern int errno;

    Save_LIB7_FPState();
    Restore_C_FPState();
    errno = 0;
    d = exp(*(PTR_CAST(double*,arg)));
    REAL_ALLOC(task,result,d);
    result = make_two_slot_record(task, result, TAGGED_INT_FROM_C_INT(errno));
    Restore_LIB7_FPState();
    return result;
}

Val c_log(Task *task, Val arg)
{
    double d;
    Val result;

    Save_LIB7_FPState();
    Restore_C_FPState();
    d = log(*(PTR_CAST(double*,arg)));
    REAL_ALLOC(task,result,d);
    Restore_LIB7_FPState();
    return result;
}

Val c_atan(Task *task, Val arg)
{
    double d;
    Val result;

    Save_LIB7_FPState();
    Restore_C_FPState();
    d = atan(*(PTR_CAST(double*,arg)));
    REAL_ALLOC(task,result,d);
    Restore_LIB7_FPState();
    return result;
}

Val c_sqrt(Task *task, Val arg)
{
    double d;
    Val result;

    Save_LIB7_FPState();
    Restore_C_FPState();
    d = sqrt(*(PTR_CAST(double*,arg)));
    REAL_ALLOC(task,result,d);
    Restore_LIB7_FPState();
    return result;
}




// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

