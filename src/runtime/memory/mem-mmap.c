/* mem-mmap.c
 *
 * Memory sub-system for systems that provide mmap.
 */

/*
###              "Do we walk in legends
###               or on the green earth
###               in the daylight?"
###
###                       -- Aragorn
 */

#include "../config.h"

#include "runtime-unixdep.h"
#include "runtime-osdep.h"
#include <sys/mman.h>

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "runtime-base.h"
#include "memory.h"

#if !(defined(HAS_MMAP) || defined(HAS_ANON_MMAP))
#  error expected HAS_MMAP or HAS_ANON_MMAP
#endif

/* protection mode for mmap memory */
#define PROT_ALL	(PROT_READ|PROT_WRITE|PROT_EXEC)

/* flags for mmap */
#ifdef HAS_ANON_MMAP
#  define MMAP_FLGS	(MAP_ANONYMOUS|MAP_PRIVATE)
#else
#  define MMAP_FLGS	MAP_PRIVATE
#endif

#  define MMAP_ADDR	0


struct heap_chunk {
    Word_t	*base;	  /* the base address of the chunk. */
    Addr_t	sizeB;	  /* the chunk's size (in bytes) */
#ifdef HAS_PARTIAL_MUNMAP
#   define	mapBase		base
#   define	mapSizeB	sizeB
#else
    Word_t	*mapBase; /* base address of the mapped region containing */
			  /* the chunk */
    Addr_t	mapSizeB; /* the size of the mapped region containing  */
			  /* the chunk */
#endif
};

extern int	errno;

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
 * Return the address of the mapped memory, or NULL on failure.
 */
static status_t MapMemory ( heap_chunk_t* chunk, Addr_t szb)
{
    int		fd;
    Addr_t	addr, offset;

#ifdef HAS_ANON_MMAP
    fd = -1;
#else
  /* Note: we use O_RDONLY, because some OS are configured such that /dev/zero
   * is not writable.  This works because we are using MAP_PRIVATE as the
   * mapping mode.
   */
    if ((fd = open("/dev/zero", O_RDONLY)) == -1) {
	Error ("unable to open /dev/zero, errno = %d\n", errno);
	return FAILURE;
    }
#endif

    /* We grab an extra BIBOP_PAGE_SZB bytes
     * to give us some room for alignment:
     */
    addr = (Addr_t) mmap (MMAP_ADDR, szb+BIBOP_PAGE_SZB, PROT_ALL, MMAP_FLGS, fd, 0);
    if (addr == -1) {
	Error ("unable to map %d bytes, errno = %d\n", szb, errno);
#ifndef HAS_ANON_MMAP
	close (fd); /* NOTE: this call clobbers errno */
#endif
	return FAILURE;
    }
#ifndef HAS_ANON_MMAP
    close (fd);
#endif

    /* Ensure BIBOP_PAGE_SZB alignment: */
    offset = BIBOP_PAGE_SZB - (addr & (BIBOP_PAGE_SZB-1));
#ifdef HAS_PARTIAL_MUNMAP
    if (offset != BIBOP_PAGE_SZB) {
      /* align addr and discard unused portions of memory */
	munmap ((void *)addr, offset);
	addr += offset;
	munmap ((void *)(addr+szb), BIBOP_PAGE_SZB-offset);
    }
    else {
	munmap ((void *)(addr+szb), BIBOP_PAGE_SZB);
    }
#else
    chunk->mapBase = (Word_t *)addr;
    chunk->mapSizeB = szb+BIBOP_PAGE_SZB;
    addr += offset;
#endif
    chunk->base = (Word_t *)addr;
    chunk->sizeB = szb;

    return SUCCESS;

} /* end of MapMemory */

/* UnmapMemory:
 *
 * Unmap a szb byte chunk of virtual memory at addr.
 */
static void UnmapMemory( heap_chunk_t* chunk )
{
    if (munmap((caddr_t)(chunk->mapBase), chunk->mapSizeB) == -1) {
	Die ("error unmapping [%#x, %#x), errno = %d\n",
	    chunk->mapBase, (Addr_t)(chunk->mapBase) + chunk->mapSizeB, errno);
    }

} /* end of UnmapMemory */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

