/* addr-hash.c
 *
 * Hash tables for mapping addresses to chunks.
 */

#include "runtime-base.h"
#include "addr-hash.h"

typedef struct item {	    /* Hash table items.                        */
    Addr_t	    addr;   /* The address on which the chunk is keyed. */
    void*	    chunk;  /* The chunk.                               */
    struct item*    next;   /* The next item in the bucket.             */
} item_t;

struct addr_table {
    int		    ignoreBits;
    int		    size;
    int		    vals_count;
    Addr_t	    mask;
    item_t**	    buckets;
};

#define HASH(table,addr)	(((addr) >> (table)->ignoreBits)   &   (table)->mask)

addr_table_t*   MakeAddrTable   (int ignoreBits, int size)
{
    /* Allocate an address hash table. */

    unsigned int    nBuckets;
    int		    i;
    addr_table_t	    *table;

    /* Find smallest power of 2
     * (but at least 16) that is
     * greater than size:
     */
    for (nBuckets = 16;  nBuckets < size;  nBuckets <<= 1)   continue;

    table		= NEW_CHUNK(addr_table_t);
    table->buckets	= NEW_VEC(item_t *, nBuckets);
    table->ignoreBits	= ignoreBits;
    table->size		= nBuckets;
    table->mask		= nBuckets-1;
    table->vals_count	= 0;
    for (i = 0;  i < nBuckets;  i++) {
	table->buckets[i] = NULL;
    }

    return table;
}



void   AddrTableInsert   (   addr_table_t* table,
                             Addr_t        addr,
                             void*         chunk
                         )
{   /* Insert an chunk into a address hash table. */

    int		h = HASH( table, addr );
    item_t*	p;

    for (   p = table->buckets[h];
            (p != NULL) && (p->addr != addr);
            p = p->next
        )
	continue;

    if (p == NULL) {
	p		    = NEW_CHUNK( item_t );
	p->addr		    = addr;
	p->chunk	    = chunk;
	p->next		    = table->buckets[h];
	table->buckets[ h ] = p;
	table->vals_count++;

    } else {
        if (p->chunk != chunk) {
	     Die ("AddrTableInsert: %#x mapped to multiple chunks", addr);
         }
    }
}



void*   AddrTableLookup   (   addr_table_t* table,
                              Addr_t        addr
                          )
{
    /* Return the chunk associated with the given address.
     * Return NULL if not found.
     */

    int		h = HASH(table,addr);
    item_t*	p;

    for (   p = table->buckets[h];
            (p != NULL) && (p->addr != addr);
             p = p->next
        )
	continue;

    if (p)   return p->chunk;
    else     return NULL;
}



void   AddrTableApply   (   addr_table_t* table,
                            void*         clos,
                            void (*f) (Addr_t, void *, void *)
                        )
{
    /* Apply the given function
     * to the elements of the table.
     */

    int		i;
    item_t	*p;

    for (i = 0;  i < table->size;  i++) {
	for (p = table->buckets[i];  p != NULL;  p = p->next) {
	    (*f) (p->addr, clos, p->chunk);
	}
    }

}



void   FreeAddrTable   (   addr_table_t*   table,
                           bool_t          freeChunks
                       )
{
    /* Deallocate the space for an address table.
     * If freeChunks is true, also deallocate the chunks.
     */
    int		i;
    item_t*	p;
    item_t*	q;

    for (i = 0;  i < table->size;  i++) {
	for (p = table->buckets[i];  p != NULL;  ) {
	    q = p->next;
	    if (freeChunks)   FREE (p->chunk);
	    FREE (p);
	    p = q;
	}
    }

    FREE (table->buckets);
    FREE (table);
}


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
