/* ccalls.h 
 *
 */

#ifndef _C_CALLS_
#define _C_CALLS_

#define N_ARGS 15 /* max number of args a Lib7 callable C function may have */

#ifndef _ASM_

#include "sizes-of-some-c-types--autogenerated.h"

/* malloc's should return sufficiently aligned blocks */
#define HAS_ALIGNED_MALLOC
#if defined(HAS_ALIGNED_MALLOC)
#include <stdlib.h>

#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#define memalign(align,size) malloc(size)
#endif

#include <string.h>

extern Vunt *checked_memalign(int n,int align);
#define checked_alloc(n) checked_memalign((n),(1))

extern Vunt mk_C_function(Task *task,
			    Val f,
			    int nargs,char *argtypes[],char *rettype);

extern Val convert_c_value_to_mythryl(Task *task,char *type,Vunt p,Val *root);
extern int convert_mythryl_value_to_c(Task *task,char **t,Vunt **p,Val ret);
extern Val revLib7List(Val l,Val acc);

extern void   set_visible_task   (Task* visible_task);

#endif

#endif /* !_C_CALLS_ */



/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
 * released per terms of SMLNJ-COPYRIGHT.
 */

