// get-quire-from-mach.c
//
// Memory sub-system for the MACH operating system.

// 'quire' (pronounced like choir) designates a multipage ram region
//         allocated directly from the host operating system.
//
//     quire: 2. A collection of leaves of parchment or paper,
//               folded one within the other, in a manuscript or book.
//                     -- http://www.thefreedictionary.com/quire

#include "../mythryl-config.h"

#include "system-dependent-unix-stuff.h"
#include <mach/mach_types.h>
#include "runtime-base.h"
#include "get-quire-from-os.h"
#include "sibid.h"

#ifndef HAS_VM_ALLOCATE
#  error expected HAS_VM_ALLOCATE
#endif

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
    Val_Sized_Unt*  base;	 		// Base address of the chunk.	SEE ABOVE WARNING!
    Val_Sized_Unt   bytesize;	 		// Chunk's size (in bytes).	SEE ABOVE WARNING!
    Val_Sized_Unt*  mapBase; 			// Base address of the mapped region containing the chunk.
    Val_Sized_Unt   mapSizeB;			// The size of the mapped region containing the chunk.
};

#define ALLOC_HEAPCHUNK()		MALLOC_CHUNK( Quire )
#define RETURN_QUIRE_TO_OS(p)		FREE(p)

#include "get-quire-from-os-stuff.c"

void  set_up_quire_os_interface  () {				// Part of the api defined by	src/c/h/get-quire-from-os.h
    //========================================
    //
    // We are invoked (only) from   set_up_heap   in:
    //     src/c/heapcleaner/heapcleaner-initialization.c
    //
    InitMemory();
}


static Status   map_quire   (Quire* chunk,  Punt bytesize) {
    // 
    // Map a   BOOK_BYTESIZE
    // aligned chunk of bytesize bytes of virtual memory.
    //
    // Return the address of the mapped memory
    // or NULL on failure.

    Punt		addr, offset;
    kern_return_t	status;

    status = vm_allocate(task_self(), &addr, bytesize+BOOK_BYTESIZE, TRUE);

    if (status) {
	errno = status;
	return FAILURE;
    }

    // Ensure BOOK_BYTESIZE alignment
    //
    offset = BOOK_BYTESIZE - (addr & (BOOK_BYTESIZE-1));
    //
    if (offset == 0) {
	//
	vm_deallocate (task_self(), addr+bytesize, BOOK_BYTESIZE);

    } else {

        // Align addr and discard unused portions of memory.
	vm_deallocate (task_self(), addr, offset);
	addr += offset;
	vm_deallocate (task_self(), addr+bytesize, BOOK_BYTESIZE-offset);
    }

    chunk->base = (Val_Sized_Unt *)addr;
    chunk->bytesize = bytesize;

    return SUCCESS;
}								// fun map_quire


static void   unmap_quire   (Quire* chunk) {
    // 
    // Unmap a chunk of virtual memory at addr.
    
    kern_return_t	status;

    status = vm_deallocate (
		task_self(),
		(vm_address_t)(chunk->base),
		(vm_size_t)(chunk->bytesize));

    if (status != KERN_SUCCESS) {
        die ("error unmapping [%#x, %#x), errno = %d\n",
	    chunk->mapBase, (Punt)(chunk->mapBase) + chunk->mapSizeB, errno);
    }
}


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


