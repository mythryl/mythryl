// get-multipage-ram-region-from-win32.c
//
// A simple memory module built on top of vmem alloc/free.
// This is currently win32 specific.


#include "../config.h"

#if defined(OPSYS_WIN32)
#include <windows.h>
#endif

#include "system-dependent-stuff.h"
#include "runtime-base.h"
#include "get-multipage-ram-region-from-os.h"
#include "sibid.h"

// struct multipage_ram_region
// The files
//     src/c/h/get-multipage-ram-region-from-os.h
//     src/c/h/heap.h
// both contain
//     typedef   struct multipage_ram_region   Multipage_Ram_Region;
// based on our definition here:
//
// WARNING:    Multipage_Ram_Region_Prefix   in   src/c/h/get-multipage-ram-region-from-os.h
// MUST be kept
// in-sync with the first two fields here!
//
struct multipage_ram_region {
    //
    Val_Sized_Unt*	base;			// Base address of the chunk.	SEE ABOVE WARNING!
    Val_Sized_Unt	bytesize;		// Chunk's size (in bytes).	SEE ABOVE WARNING!
    Val_Sized_Unt*	mapBase;		// Base address of the mapped region containing the chunk.
    Punt	mapSizeB;		// size of the mapped region containing the chunk.
};

static void* alloc_vmem();
static void  free_vmem(void *);

#define ALLOC_HEAPCHUNK()	alloc_vmem( sizeof( Multipage_Ram_Region ) )
#define RETURN_MULTIPAGE_RAM_REGION_TO_OS		free_vmem

#include "get-multipage-ram-region-from-os-stuff.c"

static void*   alloc_vmem   (int nb)   {
    // 
    // Allocate some virtual memory.
    //
    void* p =  (void*)  VirtualAlloc(NULL,
			      nb,
			      MEM_COMMIT|MEM_RESERVE,
			      PAGE_EXECUTE_READWRITE);

    if (p == NULL)   die("VirtualAlloc failed on request of size %lx\n", nb);

    return p;
}

static void   free_vmem   (void *p)   {
    //
    // Return  memory to os.

    if (!VirtualFree((LPVOID)p,
		     0,
		     MEM_RELEASE)) {
      die("unable to VirtualFree memory at %lx\n", p);
    }
}

static Status   map_multipage_ram_region   (Multipage_Ram_Region* chunk,  Punt szb) {
    //          ========================
    //
    Punt offset;
    Punt addr;

    if ((addr = (Punt) alloc_vmem(szb+BOOK_BYTESIZE)) == NULL) {
        return FAILURE;
    }

    chunk->mapBase  =  (Punt *) addr;
    chunk->mapSizeB =  szb+BOOK_BYTESIZE;

    chunk->bytesize = szb;

    offset = BOOK_BYTESIZE - (addr & (BOOK_BYTESIZE-1));

    addr += offset;

    chunk->base = (Punt *) addr;

    return SUCCESS;
}


static void   unmap_multipage_ram_region   (Multipage_Ram_Region* chunk) {
    //        ==========================
    //
    free_vmem(chunk->mapBase);
    chunk->base = chunk->mapBase = NULL;
    chunk->bytesize = chunk->mapSizeB = 0;
}

void   set_up_multipage_ram_region_os_interface   (void)   {				// Part of the api defined by	src/c/h/get-multipage-ram-region-from-os.h
    // ========================================
    //
    // We are invoked (only) from   set_up_heap   in:
    //     src/c/heapcleaner/heapcleaner-initialization.c
    //
    InitMemory();
}


// COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


