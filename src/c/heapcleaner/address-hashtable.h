// address-hashtable.h
//
// hashtables for mapping addresses to heapchunks.


#ifndef ADDRESS_HASHTABLE_H
#define ADDRESS_HASHTABLE_H

typedef   struct addr_table   Addresstable;

// Allocate an address hashtable.
//
extern Addresstable*   make_address_hashtable   (int ignore_bits, int size);					// make_address_hashtable		def in    src/c/heapcleaner/address-hashtable.c
    //                 ======================
    //
    // Called (only) from:
    //
    //     src/c/heapcleaner/datastructure-pickler-cleaner.c
    //     src/c/heapcleaner/import-heap.c



// Insert an chunk into a address hashtable.
//
extern void   addresstable_insert   (Addresstable* table,   Punt addr,   void* chunk);
    //        ===================


// Return the chunk associated with the given address.
// Return NULL if not found.
//
extern void*   addresstable_look_up   (Addresstable* table,   Punt addr);
    //         ====================


// Apply the given function to the elements of the table.
// The second argument to the function is the function's "closure," (state) and
// the third is the associated info.
//
extern void   addresstable_apply   (Addresstable* table,   void* clos,   void (*f) (Punt, void *, void *));
    //        ==================  


// Deallocate the space for an address table; if free_chunks_also is true, also deallocate
// the chunks.
//
extern void   free_address_table   (Addresstable* table,  Bool free_chunks_also);
    //        ==================

#endif // ADDRESS_HASHTABLE_H


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.


