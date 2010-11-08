/* math.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-state.h"
#include "tags.h"
#include "runtime-heap.h"
#include "ml-fp.h"

#include <math.h>

#define REAL_ALLOC(lib7_state, r, d)	{			\
	lib7_state_t	*__lib7_state = (lib7_state);			\
	lib7_val_t        *__p = __lib7_state->lib7_heap_cursor;	\
	double          *__dp;                          \
	*__p++ = DESC_reald;				\
	__dp = (double *) __p;                          \
	*__dp++ = (d);          			\
	(r) = PTR_CtoLib7(__lib7_state->lib7_heap_cursor + 1);	\
	__lib7_state->lib7_heap_cursor = (lib7_val_t *) __dp;		\
    }

lib7_val_t c_cos(lib7_state_t *lib7_state, lib7_val_t arg)
{
    double d;
    lib7_val_t result;

    Save_LIB7_FPState();
    Restore_C_FPState();
    d = cos(*(PTR_LIB7toC(double,arg)));
    REAL_ALLOC(lib7_state,result,d);
    Restore_LIB7_FPState();
    return result;
}

lib7_val_t c_sin(lib7_state_t *lib7_state, lib7_val_t arg)
{
    double d;
    lib7_val_t result;

    Save_LIB7_FPState();
    Restore_C_FPState();
    d = sin(*(PTR_LIB7toC(double,arg)));
    REAL_ALLOC(lib7_state,result,d);
    Restore_LIB7_FPState();
    return result;
}

lib7_val_t c_exp(lib7_state_t *lib7_state, lib7_val_t arg)
{
    double d;
    lib7_val_t result;
    extern int errno;

    Save_LIB7_FPState();
    Restore_C_FPState();
    errno = 0;
    d = exp(*(PTR_LIB7toC(double,arg)));
    REAL_ALLOC(lib7_state,result,d);
    REC_ALLOC2(lib7_state,result,result,INT_CtoLib7(errno));
    Restore_LIB7_FPState();
    return result;
}

lib7_val_t c_log(lib7_state_t *lib7_state, lib7_val_t arg)
{
    double d;
    lib7_val_t result;

    Save_LIB7_FPState();
    Restore_C_FPState();
    d = log(*(PTR_LIB7toC(double,arg)));
    REAL_ALLOC(lib7_state,result,d);
    Restore_LIB7_FPState();
    return result;
}

lib7_val_t c_atan(lib7_state_t *lib7_state, lib7_val_t arg)
{
    double d;
    lib7_val_t result;

    Save_LIB7_FPState();
    Restore_C_FPState();
    d = atan(*(PTR_LIB7toC(double,arg)));
    REAL_ALLOC(lib7_state,result,d);
    Restore_LIB7_FPState();
    return result;
}

lib7_val_t c_sqrt(lib7_state_t *lib7_state, lib7_val_t arg)
{
    double d;
    lib7_val_t result;

    Save_LIB7_FPState();
    Restore_C_FPState();
    d = sqrt(*(PTR_LIB7toC(double,arg)));
    REAL_ALLOC(lib7_state,result,d);
    Restore_LIB7_FPState();
    return result;
}

/* end of math.c */



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

