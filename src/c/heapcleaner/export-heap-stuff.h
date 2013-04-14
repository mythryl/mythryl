// export-heap-stuff.h


/*
###                 "A child educated only at school is an uneducated child."
###
###                                           -- George Santayana
*/



#ifndef EXPORT_HEAP_STUFF_H
#define EXPORT_HEAP_STUFF_H

#ifndef _WRITER_
    #include "writer.h"
#endif

extern Status   heapio__write_image_header	(Writer* wr,  int kind);				// heapio__write_image_header	def in    src/c/heapcleaner/export-heap-stuff.c
extern int      heapio__write_cfun_table	(Writer* wr,  Heapfile_Cfun_Table* table);		// heapio__write_cfun_table	def in    src/c/heapcleaner/export-heap-stuff.c

#endif // EXPORT_HEAP_STUFF_H


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

