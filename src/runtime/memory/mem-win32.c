/* mem-win32.c
 *
 * A simple memory module built on top of vmem alloc/free.
 * This is currently win32 specific.
 */

#include "../config.h"

#if defined(OPSYS_WIN32)
#include <windows.h>
#endif

#include "runtime-osdep.h"
#include "runtime-base.h"
#include "memory.h"

struct heap_chunk {
    Word_t	*base;	  /* the base address of the chunk. */
    Word_t	sizeB;	  /* the chunk's size (in bytes) */
    Word_t	*mapBase; /* base address of the mapped region containing */
			  /* the chunk */
    Addr_t	mapSizeB; /* the size of the mapped region containing  */
			  /* the chunk */
};

#define HEAP_CHUNK_SZB  (sizeof(heap_chunk_t))

static void *alloc_vmem();
static void free_vmem(void *);

#define ALLOC_HEAP_CHUNK()		alloc_vmem(HEAP_CHUNK_SZB)
#define FREE_HEAP_CHUNK		free_vmem

#include "mem-common.ins"

/* alloc_vmem:
 * Allocate some virtual memory.
 */
static void *alloc_vmem(int nb)
{
  void *p;

  p = (void *) VirtualAlloc(NULL,
			    nb,
			    MEM_COMMIT|MEM_RESERVE,
			    PAGE_EXECUTE_READWRITE);
  if (p == NULL) {
    Die("VirtualAlloc failed on request of size %lx\n", nb);
  }
  return p;
}

/* free_vmem:
 * Return  memory to os.
 */
static void free_vmem (void *p)
{
  if (!VirtualFree((LPVOID)p,
		   0,
		   MEM_RELEASE)) {
    Die("unable to VirtualFree memory at %lx\n", p);
  }
    
}

static status_t MapMemory (heap_chunk_t *chunk, Addr_t szb)
{
  Addr_t offset, addr;

  if ((addr = (Addr_t) alloc_vmem(szb+BIBOP_PAGE_SZB)) == NULL) {
    return FAILURE;
  }
  chunk->mapBase = (Addr_t *) addr;
  chunk->mapSizeB = szb+BIBOP_PAGE_SZB;
  chunk->sizeB = szb;
  offset = BIBOP_PAGE_SZB - (addr & (BIBOP_PAGE_SZB-1));
  addr += offset;
  chunk->base = (Addr_t *) addr;

  return SUCCESS;
}

static void UnmapMemory (heap_chunk_t* chunk)
{
  free_vmem(chunk->mapBase);
  chunk->base = chunk->mapBase = NULL;
  chunk->sizeB = chunk->mapSizeB = 0;
}

/* MEM_InitMemory:
 */
void MEM_InitMemory ()
{
    InitMemory();
} /* MEM_InitMemory */

/* end of mem-vmem.c */



/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

