/* c-calls.h 
 *
 */

#ifndef _C_CALLS_
#define _C_CALLS_

#define N_ARGS 15 /* max number of args a Lib7 callable C function may have */

#ifndef _ASM_

#include "runtime-sizes.h"

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

extern Word_t *checked_memalign(int n,int align);
#define checked_alloc(n) checked_memalign((n),(1))

extern Word_t mk_C_function(lib7_state_t *lib7_state,
			    lib7_val_t f,
			    int nargs,char *argtypes[],char *rettype);

extern lib7_val_t datumCtoLib7(lib7_state_t *lib7_state,char *type,Word_t p,lib7_val_t *root);
extern int datumMLtoC(lib7_state_t *lib7_state,char **t,Word_t **p,lib7_val_t ret);
extern lib7_val_t revLib7List(lib7_val_t l,lib7_val_t acc);

extern lib7_state_t *visible_lib7_state;
#endif

#endif /* !_C_CALLS_ */



/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

