/* heap-io.h
 *
 * Interface to heap-io library.
 */

#ifndef _HEAP_IO_
#define _HEAP_IO_

#include <stdio.h>

extern status_t ExportHeapImage (lib7_state_t *lib7_state, FILE *file);
extern status_t ExportFnImage (lib7_state_t *lib7_state, lib7_val_t funct, FILE *file);
extern lib7_state_t *ImportHeapImage (const char *fname, heap_params_t *heapParams);

extern lib7_val_t BlastOut (lib7_state_t *lib7_state, lib7_val_t chunk);
extern lib7_val_t BlastIn (lib7_state_t *lib7_state, Byte_t *data, long len, bool_t *seen_error);

#endif /* _HEAP_IO_ */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

