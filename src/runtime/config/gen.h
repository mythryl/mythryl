/* gen.h
 *
 */

#ifndef _GEN_COMMON_
#define _GEN_COMMON_

#include <stdio.h>

extern FILE *OpenFile (char *fname, char *flag);
extern void CloseFile (FILE *f, char *flag);

#ifndef _LIB7_BASE_

/* aliases for malloc/free, so that we can easily replace them */
#define MALLOC(size)	malloc(size)
#define FREE(p)		free(p)

/* Allocate a new C chunk of type t. */
#define NEW_CHUNK(t)	((t *)MALLOC(sizeof(t)))
/* Allocate a new C array of type t chunks. */
#define NEW_VEC(t,n)	((t *)MALLOC((n)*sizeof(t)))
#endif /* !_LIB7_BASE_ */

#endif /* !_GEN_COMMON_ */



/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
