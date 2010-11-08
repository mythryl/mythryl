/* memory.h
 *
 * An OS independent view of memory.  This supports allocation of
 * memory chunks aligned to BIBOP_PAGE_SZB byte boundries (see bibop.h).
 */

#ifndef _MEMORY_
#define _MEMORY_

/* The header of a heap_chunk_t package.  The full representation
 * of this depends on the underlying OS memory system, and thus is
 * abstract.
 */
struct heap_chunk_hdr {
    Addr_t	base;	  /* the base address of the chunk. */
    Addr_t	sizeB;	  /* the chunk's size (in bytes) */
};

typedef struct heap_chunk heap_chunk_t;

extern void MEM_InitMemory ();
extern heap_chunk_t* allocate_heap_chunk (Word_t szb);
extern void free_heap_chunk (heap_chunk_t *chunk);

#define HEAP_CHUNK_BASE(chunkPtr)	(((struct heap_chunk_hdr*)(chunkPtr))->base)
#define HEAP_CHUNK_SZB(chunkPtr)	(((struct heap_chunk_hdr*)(chunkPtr))->sizeB)

#ifdef _VM_STATS_
extern long MEM_GetVMSize ();
#endif

#endif /* !_MEMORY_ */



/* COPYRIGHT (c) 1992 AT&T Bell Laboratories
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

