// address-hashtable.c
//
// hashtables for mapping addresses to chunks.

#include "runtime-base.h"
#include "address-hashtable.h"

// Item
//
typedef struct item {	    		// hashtable items.
    Punt	    addr;		// The address on which the chunk is keyed.
    void*	    chunk;		// The chunk.
    struct item*    next;   		// The next item in the bucket.
} Item;

struct addr_table {
    int		    ignore_bits;
    int		    bucket_count;	// Always a power of two.
    Punt	    mask;
    int		    vals_count;
    Item**	    buckets;
};

#define HASH(table,addr)	(((addr) >> (table)->ignore_bits)   &   (table)->mask)



Addresstable*   make_address_hashtable   (int ignore_bits, int buckets)   {
    //          ======================
    //
    // Allocate an address hashtable.


    Addresstable*   table;

    // Find smallest power of 2
    // (but at least 16) that is
    // greater than 'buckets':
    //
    unsigned int    bucket_count;
    //
    if (buckets < 16) {
        buckets = 16;
    }
    for (bucket_count = 16;  bucket_count < buckets;  bucket_count <<= 1);

    table		= MALLOC_CHUNK(Addresstable);
    table->buckets	= MALLOC_VEC( Item*, bucket_count);
    table->ignore_bits	= ignore_bits;
    table->bucket_count	= bucket_count;
    table->mask		= bucket_count-1;
    table->vals_count	= 0;
    //
    for (unsigned int bucket = 0;  bucket < bucket_count;  bucket++) {
	//
	table->buckets[ bucket ] = NULL;
    }

    return table;
}



void   addresstable_insert   (
    // ===================
    //
    Addresstable*  table,
    Punt           addr,
    void*          chunk
){
    // Insert a chunk into a address hashtable.

    int		h = HASH( table, addr );
    Item*	p;

    for (p = table->buckets[h];
         p != NULL  &&  p->addr != addr;
         p = p->next
        );


    if (!p) {
	//
	p		    = MALLOC_CHUNK( Item );
	p->addr		    = addr;
	p->chunk	    = chunk;
	p->next		    = table->buckets[h];
	table->buckets[ h ] = p;
	table->vals_count++;

    } else {

        if (p->chunk != chunk) {
	     die ("addresstable_insert: %#x mapped to multiple chunks", addr);
         }
    }
}



void*   addresstable_look_up   (
    //  ====================
    //
    Addresstable*  table,
    Punt           addr
){
    // Return the chunk associated with the given address.
    // Return NULL if not found.

    int h = HASH(table,addr);

    Item* p;
    for (p = table->buckets[h];
         p != NULL  &&  p->addr != addr;
         p = p->next
        )
	continue;

    if (p)   return p->chunk;
    else     return NULL;
}



void   addresstable_apply   (
    // ==================
    //
    Addresstable* table,
    void*         clos,
    void (*       f       ) (Punt, void*, void*)
){
    // Apply the given function
    // to the elements of the table.

    for (int bucket = 0;   bucket < table->bucket_count;   bucket++) {
	//
	for (Item*
	    p  =  table->buckets[ bucket ];
	    p !=  NULL;
	    p  =  p->next
	){
	    (*f) (p->addr, clos, p->chunk);
	}
    }

}



void   free_address_table   (
    // ==================
    //
    Addresstable*   table,
    Bool            free_chunks_also
){
    // Deallocate the space for an address table.
    // If free_chunks_also is true, also deallocate the chunks.

    for (int bucket = 0;  bucket < table->bucket_count;  bucket++) {
	//
	for(Item* p = table->buckets[bucket];  p != NULL;  ) {
	    Item* q = p->next;

	    if (free_chunks_also)   FREE (p->chunk);

	    FREE( p );

	    p = q;
	}
    }

    FREE (table->buckets);
    FREE (table);
}


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

