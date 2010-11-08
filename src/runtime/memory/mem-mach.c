/* mem-mach.c
 *
 * Memory sub-system for the MACH operating system.
 *
 */


/*  ###             "A C program is like a fast dance
    ###              on a newly waxed dance floor
    ###              by people carrying razors."
    ###
    ###                         -- Waldi Ravens
 */


#include "../config.h"

#include "runtime-unixdep.h"
#include <mach/mach_types.h>
#include "runtime-base.h"
#include "memory.h"

#ifndef HAS_VM_ALLOCATE
#  error expected HAS_VM_ALLOCATE
#endif

struct heap_chunk {
    Word_t	*base;	  /* the base address of the chunk. */
    Word_t	sizeB;	  /* the chunk's size (in bytes) */
    Word_t	*mapBase; /* base address of the mapped region containing */
			  /* the chunk */
    Word_t	mapSizeB; /* the size of the mapped region containing  */
			  /* the chunk */
};

#define ALLOC_HEAP_CHUNK()		NEW_CHUNK( heap_chunk_t )
#define FREE_HEAP_CHUNK(p)		FREE(p)

#include "mem-common.ins"

/* MEM_InitMemory:
 */
void MEM_InitMemory ()
{
    InitMemory();

} /* MEM_InitMemory */


/* MapMemory:
 *
 * Map a BIBOP_PAGE_SZB aligned chunk of szb bytes of virtual memory.
 * Return the address of the mapped memory (or NULL on failure).
 */
static status_t MapMemory ( heap_chunk_t* chunk, Addr_t szb)
{
    Addr_t		addr, offset;
    kern_return_t	status;

    status = vm_allocate(task_self(), &addr, szb+BIBOP_PAGE_SZB, TRUE);

    if (status) {
	errno = status;
	return FAILURE;
    }

  /* insure BIBOP_PAGE_SZB alignment */
    offset = BIBOP_PAGE_SZB - (addr & (BIBOP_PAGE_SZB-1));
    if (offset != 0) {
      /* align addr and discard unused portions of memory */
	vm_deallocate (task_self(), addr, offset);
	addr += offset;
	vm_deallocate (task_self(), addr+szb, BIBOP_PAGE_SZB-offset);
    }
    else {
	vm_deallocate (task_self(), addr+szb, BIBOP_PAGE_SZB);
    }

    chunk->base = (Word_t *)addr;
    chunk->sizeB = szb;

    return SUCCESS;

} /* end of MapMemory */

/* UnmapMemory:
 *
 * Unmap a chunk of virtual memory at addr.
 */
static void UnmapMemory (heap_chunk_t* chunk)
{
    kern_return_t	status;

    status = vm_deallocate (
		task_self(),
		(vm_address_t)(chunk->base),
		(vm_size_t)(chunk->sizeB));

    if (status != KERN_SUCCESS) {
        Die ("error unmapping [%#x, %#x), errno = %d\n",
	    chunk->mapBase, (Addr_t)(chunk->mapBase) + chunk->mapSizeB, errno);
    }

} /* end of UnmapMemory */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

