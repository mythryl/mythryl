// get-multipage-ram-region-from-os-stuff.c
//
// 'quire' (pronounced like choir) designates a multipage ram region
//         allocated directly from the host operating system.
//
//     quire: 2. A collection of leaves of parchment or paper,
//               folded one within the other, in a manuscript or book.
//                     -- http://www.thefreedictionary.com/quire
//
// Code common to all three implementations
// of the  memory management library.
//
// We do not get directly compiled;
// rather we get included by:
//
//     src/c/ram/get-multipage-ram-region-from-win32.c
//     src/c/ram/get-multipage-ram-region-from-mmap.c
//     src/c/ram/get-multipage-ram-region-from-mach.c
//
// Those files make available to us several definitions,
// each of which varies between those three files:
//     static void   unmap_multipage_ram_region   (Multipage_Ram_Region* chunk);
//     static Status   map_multipage_ram_region   (Multipage_Ram_Region* chunk,  Punt bytesize);
//     struct multipage_ram_region { ... }
//     #define ALLOC_HEAPCHUNK()	...
//     #define RETURN_MULTIPAGE_RAM_REGION_TO_OS(p)	...


#ifndef SHARED_MEMORY_MANAGEMENT_CODE_C
#define SHARED_MEMORY_MANAGEMENT_CODE_C

static Punt	PageSize;	// The system page size.
static Punt	PageShift;	// PageSize == (1 << PageShift)
static Punt	VMSizeB;	// The amount of virtual memory allocated.

static Status   map_multipage_ram_region   (Multipage_Ram_Region* chunk,  Punt bytesize);
static void   unmap_multipage_ram_region   (Multipage_Ram_Region* chunk);


static void   InitMemory   ()   {
    //
    // Initialize the common stuff.

    VMSizeB  =  0;

    PageSize =  GET_HOST_HARDWARE_PAGE_BYTESIZE();			// GET_HOST_HARDWARE_PAGE_BYTESIZE		def in   src/c/h/system-dependent-stuff.h

    int j = 0;

    for (int i = 1;  i != PageSize;  i <<= 1, j++)   continue;

    PageShift = j;
}


Multipage_Ram_Region*   obtain_multipage_ram_region_from_os   (Val_Sized_Unt bytesize) {
    //       ====================	
    //
    // Get a new memory chunk from the OS.
    // Return a pointer to the chunk descriptor,
    // or NULL on failure.
    //
    // We get invoked from three places:
    //    src/c/heapcleaner/heapcleaner-stuff.c
    //    src/c/heapcleaner/heapcleaner-initialization.c
    //    src/c/heapcleaner/hugechunk.c

    Val_Sized_Unt alloc_bytesize;

    Multipage_Ram_Region*	chunk;

    if ((chunk = ALLOC_HEAPCHUNK()) == NULL) {
	//
	say_error( "Unable to malloc chunk descriptor\n" );
	//
	return NULL;
    }

    alloc_bytesize
	=
        (bytesize <= BOOK_BYTESIZE)
	    ?
	    BOOK_BYTESIZE
            :
            BOOKROUNDED_BYTESIZE( bytesize );

									// map_multipage_ram_region	def in   src/c/ram/get-multipage-ram-region-from-win32.c
									// map_multipage_ram_region	def in   src/c/ram/get-multipage-ram-region-from-mmap.c
									// map_multipage_ram_region	def in   src/c/ram/get-multipage-ram-region-from-mach.c
    if (map_multipage_ram_region (chunk, alloc_bytesize) == FAILURE) {
	//
	RETURN_MULTIPAGE_RAM_REGION_TO_OS( chunk );
	return NULL;
    }

    VMSizeB += alloc_bytesize;

    return chunk;
}



void   return_multipage_ram_region_to_os   (Multipage_Ram_Region* chunk) {
    // ============================
    //
    if (!chunk)  	return;

    unmap_multipage_ram_region( chunk );

    VMSizeB -= chunk->bytesize;

    RETURN_MULTIPAGE_RAM_REGION_TO_OS( chunk );
}

#endif							// SHARED_MEMORY_MANAGEMENT_CODE_C
