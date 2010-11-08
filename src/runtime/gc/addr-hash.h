/* addr-hash.h
 *
 * Hash tables for mapping addresses to chunks.
 */

#ifndef _ADDR_HASH_
#define _ADDR_HASH_

typedef struct addr_table addr_table_t;

/* Allocate an address hash table.
 */
extern addr_table_t*   MakeAddrTable   (int ignoreBits, int size);

/* Insert an chunk into a address hash table.
 */
extern void   AddrTableInsert   (addr_table_t *table,   Addr_t addr,   void* chunk);

/* Return the chunk associated with the given address.
 * Return NULL if not found.
 */
extern void*   AddrTableLookup   (addr_table_t* table,   Addr_t addr);

/* Apply the given function to the elements of the table.  The second
 * argument to the function is the function's "closure," and the third is
 * the associated info.
 */
extern void   AddrTableApply   (addr_table_t* table,   void* clos,   void (*f) (Addr_t, void *, void *));

/* Deallocate the space for an address table; if freeChunks is true, also deallocate
 * the chunks.
 */
extern void   FreeAddrTable   (addr_table_t* table, bool_t freeChunks);

#endif /* !_ADDR_HASH_ */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

