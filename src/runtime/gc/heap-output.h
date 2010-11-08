/* heap-output.h
 *
 */



/*
###                 "A child educated only at school is an uneducated child."
###
###                                           -- George Santayana
 */



#ifndef _HEAP_OUTPUT_
#define _HEAP_OUTPUT_

#ifndef _WRITER_
#include "writer.h"
#endif

extern status_t HeapIO_WriteImageHeader (writer_t *wr, int kind);
extern Addr_t HeapIO_WriteExterns (writer_t *wr, export_table_t *table);

#endif /* !_HEAP_OUTPUT_ */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
