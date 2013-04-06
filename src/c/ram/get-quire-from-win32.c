// get-quire-from-win32.c
//
// A simple memory module built on top of vmem alloc/free.
// This is currently win32 specific.

// 'quire' (pronounced like choir) designates a multipage ram region
//         allocated directly from the host operating system.
//
//     quire: 2. A collection of leaves of parchment or paper,
//               folded one within the other, in a manuscript or book.
//                     -- http://www.thefreedictionary.com/quire

#include "../mythryl-config.h"

#if defined(OPSYS_WIN32)
#include <windows.h>
#endif

#include "system-dependent-stuff.h"
#include "runtime-base.h"
#include "get-quire-from-os.h"
#include "sibid.h"

// struct quire
// The files
//     src/c/h/get-quire-from-os.h
//     src/c/h/heap.h
// both contain
//     typedef   struct quire   Quire;
// based on our definition here:
//
// WARNING:    Quire_Prefix   in   src/c/h/get-quire-from-os.h
// MUST be kept
// in-sync with the first two fields here!
//
struct quire {
    //
    Vunt*	base;			// Base address of the chunk.	SEE ABOVE WARNING!
    Vunt	bytesize;		// Chunk's size (in bytes).	SEE ABOVE WARNING!
    Vunt*	mapBase;		// Base address of the mapped region containing the chunk.
    Vunt	mapSizeB;		// size of the mapped region containing the chunk.
};

static void* alloc_vmem();
static void  free_vmem(void *);

#define ALLOC_HEAPCHUNK()	alloc_vmem( sizeof( Quire ) )
#define RETURN_QUIRE_TO_OS		free_vmem

#include "get-quire-from-os-stuff.c"

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

static Status   map_quire   (Quire* chunk,  Vunt szb) {
    //          ========================
    //
    Vunt offset;
    Vunt addr;

    if ((addr = (Vunt) alloc_vmem(szb+BOOK_BYTESIZE)) == NULL) {
        return FALSE;
    }

    chunk->mapBase  =  (Vunt *) addr;
    chunk->mapSizeB =  szb+BOOK_BYTESIZE;

    chunk->bytesize = szb;

    offset = BOOK_BYTESIZE - (addr & (BOOK_BYTESIZE-1));

    addr += offset;

    chunk->base = (Vunt *) addr;

    return TRUE;
}


static void   unmap_quire   (Quire* chunk) {
    //        ==========================
    //
    free_vmem(chunk->mapBase);
    chunk->base = chunk->mapBase = NULL;
    chunk->bytesize = chunk->mapSizeB = 0;
}

void   set_up_quire_os_interface   (void)   {				// Part of the api defined by	src/c/h/get-quire-from-os.h
    // ========================================
    //
    // We are invoked (only) from   set_up_heap   in:
    //     src/c/heapcleaner/heapcleaner-initialization.c
    //
    InitMemory();
}


// COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.


