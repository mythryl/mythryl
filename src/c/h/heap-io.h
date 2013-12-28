// heap-io.h
//
// Interface to heap-io library.


#ifndef HEAP_IO_H
#define HEAP_IO_H

#include <stdio.h>

#include "heap.h"

extern Status   export_heap_image		(Task* task,  FILE* file);						// export_heap_image			def in   src/c/heapcleaner/export-heap.c
extern Status   export_fn_image			(Task* task,  Val funct,  FILE* file);					// export_fn_image			def in   src/c/heapcleaner/export-heap.c

extern Task*    import_heap_image__may_heapclean(const char* fname,  Heapcleaner_Args* cleaner_args, Roots*);		// import_heap_image__may_heapclean	def in   src/c/heapcleaner/import-heap.c

extern Val	pickle_datastructure		(Task* task,  Val chunk);						// pickle_datastructure			def in   src/c/heapcleaner/datastructure-pickler.c
extern Val	unpickle_datastructure		(Task* task,  Unt8* data,  long len,  Bool* seen_error);		// unpickle_datastructure		def in   src/c/heapcleaner/datastructure-unpickler.c

#endif // HEAP_IO_H


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.


