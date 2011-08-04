// import-heap-stuff.h


/*
###                 "The Navy is very old and very wise."
###
###                               -- Rudyard Kipling,
###                                  The Fringes of the Fleet
*/


#ifndef HEAP_INPUT_H
#define HEAP_INPUT_H

#include <stdio.h>

#include "address-hashtable.h"

// An input source for reading heap data.  We need
// this because the datastructure pickler may need
// to read from a stream that has already had bytes
// read from it.
//
typedef struct {
    //
    Bool   needs_to_be_byteswapped;	// TRUE iff the input bytes need to be swapped.
    //
    FILE*  file;			// The file descriptor to read once the	 buffered characters are exhausted.
    //
    Unt8*  base;			// Start of the buffered characters.
    Unt8*  buf;				// Current position in the buffer.
    long   nbytes;
    //
} Inbuf;


//////////////////////////////////////////
// Hugechunk relocation info:
//
typedef struct {
    //
    Punt	old_address;
    Hugechunk*  new_chunk;
    //
} Hugechunk_Relocation_Info;

typedef struct {
    //
    Punt  first_ram_quantum;		// Address of the first page of the region.
    int   page_count;			// Number of pages in the region.
    //
    Hugechunk_Relocation_Info**  hugechunk_page_to_hugechunk;	// The map from pages to hugechunk relocation info.
    //
} Hugechunk_Region_Relocation_Info;


inline Hugechunk_Relocation_Info*   get_hugechunk_holding_pointee_via_reloc_info   (Hugechunk_Region_Relocation_Info* region,  Val pointer)   {
    //                              ============================================
    //
    // Map an address to the corresponding Hugechunk*
    // This is a clone of
    //     get_hugechunk_holding_pointee 
    // from
    //     src/c/h/heap.h
    // except for the type of 'region'
    // and the return type:
    //
    return   region->hugechunk_page_to_hugechunk[   GET_HUGECHUNK_FOR_POINTER_PAGE( region, pointer )   ];
}



///////////////////////////////////////////
// Big-chunk region hashtable interface:
//
#define LOOK_UP_HUGECHUNK_REGION(table, book2sibid_index)    ((Hugechunk_Region_Relocation_Info*) addresstable_look_up( table, ADDRESS_OF_BOOK( book2sibid_index )))

// Utility routines:
//
extern Val*     heapio__read_externs_table  (Inbuf* bp);					// heapio__read_externs_table	def in    src/c/cleaner/import-heap-stuff.c
extern Status   heapio__seek                (Inbuf* bp,  long offset);				// heapio__seek			def in    src/c/cleaner/import-heap-stuff.c
extern Status   heapio__read_block          (Inbuf* bp,  void* blk,  long len);			// heapio__read_block		def in    src/c/cleaner/import-heap-stuff.c

#endif				// HEAP_INPUT_H


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


