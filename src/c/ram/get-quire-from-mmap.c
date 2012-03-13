// get-quire-from-mmap.c
//
// Memory sub-system for systems that provide mmap.

// 'quire' (pronounced like choir) designates a multipage ram region
//         allocated directly from the host operating system.
//
//     quire: 2. A collection of leaves of parchment or paper,
//               folded one within the other, in a manuscript or book.
//                     -- http://www.thefreedictionary.com/quire

/*
###              "Do we walk in legends
###               or on the green earth
###               in the daylight?"
###
###                       -- Aragorn
*/

#include "../mythryl-config.h"

#include "system-dependent-unix-stuff.h"
#include "system-dependent-stuff.h"
#include <sys/mman.h>

#if HAVE_FCNTL_H
    #include <fcntl.h>
#endif

#include "runtime-base.h"
#include "get-quire-from-os.h"
#include "sibid.h"

#if !(defined(HAS_MMAP) || defined(HAS_ANON_MMAP))
    #error expected HAS_MMAP or HAS_ANON_MMAP
#endif


// Flags for mmap:
//
#ifdef HAS_ANON_MMAP
#  define MMAP_FLGS	(MAP_ANONYMOUS|MAP_PRIVATE)
#else
#  define MMAP_FLGS	MAP_PRIVATE
#endif


// struct quire
// The files
//     src/c/h/get-quire-from-os.h
//     src/c/h/heap.h
// both contain
//     typedef   struct quire   Quire;
// based on our definition here.
//
// WARNING:    Quire_Prefix   in   src/c/h/get-quire-from-os.h
// MUST be kept
// in-sync with the first two fields here!
//
struct quire {
    //
    Vunt*	base;					// The base address of the region.	SEE ABOVE WARNING!
    Punt	bytesize;					// The region's size.			SEE ABOVE WARNING!

    #ifdef HAS_PARTIAL_MUNMAP
        #define	mapBase		base
        #define	mapSizeB	bytesize
    #else
	Vunt*		mapBase;			// Base address of the mapped region containing the chunk.
	Punt	mapSizeB;					// The size of the mapped region containing     the chunk.
    #endif
};

extern int	errno;

#define ALLOC_HEAPCHUNK()		MALLOC_CHUNK( Quire )
#define RETURN_QUIRE_TO_OS(p)		FREE(p)

#include "get-quire-from-os-stuff.c"



void   set_up_quire_os_interface  () {					// Part of the api defined by	src/c/h/get-quire-from-os.h
    // ========================================
    //
    // We are invoked (only) from   set_up_heap   in:
    //     src/c/heapcleaner/heapcleaner-initialization.c
    //
    InitMemory();										// From src/c/ram/get-quire-from-os-stuff.c
}


static Status   map_quire   (Quire* chunk,  Punt bytesize) {
    // 
    // Map a BOOK_BYTESIZE
    // aligned chunk of bytesize bytes of virtual memory.
    //
    // Return the address of the mapped memory
    // or NULL on failure.
    //
    // We get called (only) from
    //     src/c/ram/get-quire-from-os-stuff.c

    int fd;

    Punt	addr;
    Punt	offset;

    #ifdef HAS_ANON_MMAP
	fd = -1;
    #else
        // Note: we use O_RDONLY, because some OS are
        // configured such that /dev/zero is not writable.
        // This works because we are using MAP_PRIVATE
        // as the mapping mode.
	//
	if ((fd = open("/dev/zero", O_RDONLY)) == -1) {
	    //
	    say_error( "Unable to open /dev/zero, errno = %d\n", errno );	// strerror would be nice here and elsewhere XXX BUGGO FIXME
	    //
	    return FALSE;
	}
    #endif

    // We grab an extra BOOK_BYTESIZE bytes
    // to give us some room for alignment:
    //
    addr = (Punt)
               mmap(
                   NULL,					// Requested address at which to map new memory. NULL lets the kernel choose freely -- the most portable choice.
                   bytesize + BOOK_BYTESIZE,	// Number of bytes of ram desired from OS.
		   (PROT_READ | PROT_WRITE | PROT_EXEC),	// Requested protection mode for mmap memory. We want full read-write-execute access to it.
                   MMAP_FLGS,					// Make allocated memory private -- changes not visible to other processes.
                   fd,						// File to map into memory. Ignored (unnecessary) on systems with MAP_ANONYMOUS, else /dev/zero. Either way we get zero-initialized memory.
                   0						// Offset within file 'fd'.
               );
    //
    if (addr == -1) {
	//
	say_error( "Unable to map %d bytes, errno = %d\n", bytesize, errno );

	#ifndef HAS_ANON_MMAP
	    close (fd);				// This call clobbers errno.
	#endif

	return FALSE;
    }
    #ifndef HAS_ANON_MMAP
	close (fd);
    #endif

    // Ensure BOOK_BYTESIZE alignment:
    //
    offset = BOOK_BYTESIZE - (addr & (BOOK_BYTESIZE-1));
    //
    #ifndef HAS_PARTIAL_MUNMAP
	//
	chunk->mapBase  =  (Vunt*) addr;
	chunk->mapSizeB =  bytesize + BOOK_BYTESIZE;
	addr += offset;
    #else
	if (offset == BOOK_BYTESIZE) {
	    //
	    munmap ((void *)(addr+bytesize), BOOK_BYTESIZE);
	    //
	} else {
	    //
	    // Align addr and discard unused portions of memory:
	    //
	    munmap ((void *)addr, offset);
	    addr += offset;
	    munmap ((void *)(addr+bytesize), BOOK_BYTESIZE-offset);
	}
    #endif

    chunk->base = (Vunt *)addr;
    chunk->bytesize = bytesize;

    return TRUE;
}									// fun map_quire

static void   unmap_quire   (Quire* chunk) {
    // 
    // Unmap a szb byte chunk of virtual memory at addr.

    if (munmap((caddr_t)(chunk->mapBase), chunk->mapSizeB) == -1) {
	//
	die ("error unmapping [%#x, %#x), errno = %d\n", chunk->mapBase, (Punt)(chunk->mapBase) + chunk->mapSizeB, errno);
    }
}


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


