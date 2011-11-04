// hugechunk.c
//
// Code for managing hugechunk regions.

#include "../mythryl-config.h"

#include "runtime-base.h"
#include "get-multipage-ram-region-from-os.h"
#include "heap.h"
#include <string.h>

#ifdef BO_DEBUG



void   print_hugechunk_region_map   (Hugechunk_Region* r)   {
    // =========================
    //
    Hugechunk*	dp;
    Hugechunk*	dq;
    int			i;

    debug_say ("[%d] %d/%d, @%#x: ", r->minGen, r->nFree, r->page_count, r->first_ram_quantum);

    for (i = 0, dq = NULL;  i < r->page_count;  i++) {

	dp = r->hugechunk_page_to_hugechunk[i];

	if (dp != dq) {
	    debug_say ("|");
	    dq = dp;
	}

	if (HUGECHUNK_IS_FREE(dp))   debug_say ("_");
	else                  debug_say ("X");
    }
    debug_say ("|\n");
}

#endif


Hugechunk*   allocate_hugechunk_region   (
    //       =========================
    // 
    Heap* heap,
    Punt  bytesize
){
    // Allocate a big chunk region that is
    // large enough to hold an chunk of at
    // least bytesize bytes.
    //
    // It returns the descriptor for the
    // free hugechunk that is the region.
    //
    // NOTE: It does not mark the book_to_sibid__global entries for the region;
    //       this must be done by the caller.

    int npages;
    int old_npages;

    Punt  record_bytesize;
    Punt  ram_region_bytesize;

    Hugechunk_Region*      region;
    Multipage_Ram_Region*  ram_region;

    // Compute the memory chunk size.
    // NOTE: there probably is a closed form for this,
    // but I'm too lazy to try to figure it out.        XXX BUGGO FIXME

    npages
	=
	ROUND_UP_TO_POWER_OF_TWO (
	    //
	    bytesize,
	    HUGECHUNK_RAM_QUANTUM_IN_BYTES
        )
        >>
        LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES;

    do {
	old_npages = npages;

	record_bytesize = ROUND_UP_TO_POWER_OF_TWO(HUGECHUNK_REGION_RECORD_BYTESIZE(npages), HUGECHUNK_RAM_QUANTUM_IN_BYTES);

	bytesize = (npages << LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES);

	ram_region_bytesize = BOOKROUNDED_BYTESIZE(record_bytesize+bytesize);
	ram_region_bytesize = (ram_region_bytesize < MINIMUM_HUGECHUNK_RAM_REGION_BYTESIZE) ? MINIMUM_HUGECHUNK_RAM_REGION_BYTESIZE : ram_region_bytesize;

	npages = (ram_region_bytesize - record_bytesize) >> LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES;

    } while (npages != old_npages);

    ram_region =  obtain_multipage_ram_region_from_os(  ram_region_bytesize  );		if (!ram_region) die( "Unable to allocate hugechunk region.");

    region = (Hugechunk_Region*) BASE_ADDRESS_OF_MULTIPAGE_RAM_REGION( ram_region );


    Hugechunk* chunk = MALLOC_CHUNK( Hugechunk );						if (!chunk)	 die( "Unable to allocate hugechunk descriptor.");


    // Initialize the hugechunk record:
    //
    region->first_ram_quantum	=  (Punt) region + record_bytesize;
    //
    region->page_count	= npages;
    region->free_pages	= npages;
    //
    region->age_of_youngest_live_chunk_in_region
	=
	MAX_AGEGROUPS;

    region->ram_region		= ram_region;
    //
    region->next		= heap->hugechunk_ramregions;
    heap->hugechunk_ramregions	= region;

    heap->hugechunk_ramregion_count++;

    for (int i = 0;  i < npages;  i++) {
        //
	region->hugechunk_page_to_hugechunk[i] = chunk;
    }

    // Initialize the descriptor for the region's memory:
    //
    chunk->chunk	 	=  region->first_ram_quantum;
    chunk->bytesize 	=  bytesize;
    //
    chunk->hugechunk_state	=  FREE_HUGECHUNK;
    chunk->region	 	=  region;

    #ifdef BO_DEBUG
	debug_say ("allocate_hugechunk_region: %d pages @ %#x\n", npages, region->first_ram_quantum);
    #endif

    return chunk;
}



Hugechunk*   allocate_hugechunk   (
    //       ==================
    //
    Heap*     heap,
    int       age,
    //
    Punt    hugechunk_bytesize
) {
    // Allocate a hugechunk of the given size.

    Hugechunk*   header;
    Hugechunk*   dp;
    Hugechunk*   new_chunk;

    Hugechunk_Region* region;

    int npages;
    int first_ram_quantum;

    Punt total_bytesize
	=
	ROUND_UP_TO_POWER_OF_TWO( hugechunk_bytesize, HUGECHUNK_RAM_QUANTUM_IN_BYTES );

    npages = (total_bytesize >> LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES);

    // Search for a free chunk that is big enough (first-fit):
    //
    header = heap->hugechunk_freelist;
    //
    for (dp = header->next;  (dp != header) && (dp->bytesize < total_bytesize);  dp = dp->next);

    if (dp == header) {
	//
        // No free chunk fits, so allocate a new region:
	//
	dp = allocate_hugechunk_region( heap, total_bytesize );

	region = dp->region;

	if (dp->bytesize == total_bytesize) {
	    //
	    // Allocate the whole region to the chunk:
	    //
	    new_chunk = dp;

	} else {

	    // Split the free chunk:
	    //
	    new_chunk		= MALLOC_CHUNK(Hugechunk);
	    new_chunk->chunk	= dp->chunk;
	    new_chunk->region	= region;
	    dp->chunk		= (Punt)(dp->chunk) + total_bytesize;
	    dp->bytesize  -= total_bytesize;
	    insert_hugechunk_in_doubly_linked_list(heap->hugechunk_freelist, dp);						// insert_hugechunk_in_doubly_linked_list	def in   src/c/h/heap.h
	    first_ram_quantum	= GET_HUGECHUNK_FOR_POINTER_PAGE(region, new_chunk->chunk);

	    for (int i = 0;  i < npages;  i++) {
		//
		region->hugechunk_page_to_hugechunk[ first_ram_quantum + i ]
		    =
		    new_chunk;
            }
	}

    } else if (dp->bytesize == total_bytesize) {

	remove_hugechunk_from_doubly_linked_list(dp);						// remove_hugechunk_from_doubly_linked_list	def in   src/c/h/heap.h
	new_chunk = dp;
	region = dp->region;

    } else {

        // Split the free chunk, leaving dp in the free list:
        //
	region		   = dp->region;
	new_chunk	   = MALLOC_CHUNK(Hugechunk);
	new_chunk->chunk   = dp->chunk;
	new_chunk->region  = region;
	dp->chunk	   = (Punt)(dp->chunk) + total_bytesize;
	dp->bytesize -= total_bytesize;
	first_ram_quantum  = GET_HUGECHUNK_FOR_POINTER_PAGE(region, new_chunk->chunk);

	for (int i = 0;  i < npages;  i++) {
	    //
	    dp->region->hugechunk_page_to_hugechunk[ first_ram_quantum + i ]
		=
		new_chunk;
        }
    }

    new_chunk->bytesize   =  hugechunk_bytesize;
    new_chunk->hugechunk_state =  YOUNG_HUGECHUNK;
    new_chunk->age	       =  age;

    region->free_pages  -=  npages;

    if (region->age_of_youngest_live_chunk_in_region > age) {
	region->age_of_youngest_live_chunk_in_region = age;
	//
	set_book2sibid_entries_for_range (book_to_sibid__global, (Val*)region, BYTESIZE_OF_MULTIPAGE_RAM_REGION( region->ram_region ), HUGECHUNK_DATA_SIBID(age));

	book_to_sibid__global[ GET_BOOK_CONTAINING_POINTEE( region ) ]
	    =
	    HUGECHUNK_RECORD_SIBID( age );
    }

    #ifdef BO_DEBUG
        debug_say ("allocate_hugechunk: %d bytes @ %#x\n", hugechunk_bytesize, new_chunk->chunk);
        print_hugechunk_region_map(region);
    #endif

    return new_chunk;
}


void   free_hugechunk   (
    // ==============
    //
    Heap*     heap,
    Hugechunk* chunk
){
    // Mark a big chunk as free and add it to the free list.

    Hugechunk_Region* region =  chunk->region;

    Hugechunk* dp;

    Punt  total_bytesize
	=
	ROUND_UP_TO_POWER_OF_TWO( chunk->bytesize, HUGECHUNK_RAM_QUANTUM_IN_BYTES);

    int first_ram_quantum =  GET_HUGECHUNK_FOR_POINTER_PAGE(region, chunk->chunk);
    int last_ram_quantum         =  first_ram_quantum + (total_bytesize >> LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES);

    #ifdef BO_DEBUG
	debug_say ("free_hugechunk: @ %#x, book2sibid age = %x, age = %d, state = %d, pages=[%d..%d)\n",
	    chunk->chunk,
	    (unsigned) GET_AGE_FROM_SIBID( SIBID_FOR_POINTER( book_to_sibid__global, chunk->chunk)),
            chunk->age,
            chunk->state,
            first_ram_quantum,
            last_ram_quantum
        );
	//
	print_hugechunk_region_map(region);
    #endif

    if (first_ram_quantum > 0
    &&  HUGECHUNK_IS_FREE( region->hugechunk_page_to_hugechunk[ first_ram_quantum -1 ])
    ){
        // Coalesce with adjacent free chunk:
        //
	dp = region->hugechunk_page_to_hugechunk[ first_ram_quantum -1 ];
	//
	remove_hugechunk_from_doubly_linked_list( dp );						// remove_hugechunk_from_doubly_linked_list	def in   src/c/h/heap.h

	for (int i = GET_HUGECHUNK_FOR_POINTER_PAGE(region, dp->chunk);   i < first_ram_quantum;   i++) {
	    //
	    region->hugechunk_page_to_hugechunk[i] = chunk;
        }

	chunk->chunk = dp->chunk;

	total_bytesize += dp->bytesize;

	FREE( dp );
    }

    if (last_ram_quantum < region->page_count
    &&  HUGECHUNK_IS_FREE( region->hugechunk_page_to_hugechunk[ last_ram_quantum ])
    ){
	//
        // Coalesce with adjacent free chunk:

	dp = region->hugechunk_page_to_hugechunk[ last_ram_quantum ];

	remove_hugechunk_from_doubly_linked_list( dp );

	for (int i = last_ram_quantum, j = i+(dp->bytesize >> LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES);
             i < j;
             i++
        ){
	    //
	    region->hugechunk_page_to_hugechunk[ i ]
		=
		chunk;
        }

	total_bytesize += dp->bytesize;
	FREE (dp);
    }

    chunk->bytesize   =  total_bytesize;
    chunk->hugechunk_state =  FREE_HUGECHUNK;

    region->free_pages +=   (last_ram_quantum - first_ram_quantum);

    // What if (region->free_pages == region->page_count) ??? XXX BUGGO FIXME

    // Add chunk to freelist:
    //
    insert_hugechunk_in_doubly_linked_list( heap->hugechunk_freelist, chunk );						// insert_hugechunk_in_doubly_linked_list	def in   src/c/h/heap.h
}


Hugechunk*   address_to_hugechunk   (Val addr) {
    //       ====================
    //
    // Given an address within a hugechunk,
    // return the chunk's descriptor record.

    Sibid*    book2sibid = book_to_sibid__global;
    Sibid    sibid;
    Hugechunk_Region* rp;

    {   int  i;
	for (i = GET_BOOK_CONTAINING_POINTEE(addr);  !SIBID_ID_IS_BIGCHUNK_RECORD( sibid = book2sibid[ i ]);  i--);

	rp =  (Hugechunk_Region*) ADDRESS_OF_BOOK( i );
    }

    return get_hugechunk_holding_pointee( rp, addr );
}



Unt8*   codechunk_comment_string_for_program_counter   (Val_Sized_Unt  program_counter)   {
    //  ============================================
    //
    // Return the comment string of the codechunk
    // containing the given program counter, or else NULL.
    //
    // This fn is called (only) from:
    //
    //     src/c/main/run-mythryl-code-and-runtime-eventloop.c	

    

    Sibid sib_id =  SIBID_FOR_POINTER( book_to_sibid__global, program_counter );

    if (!SIBID_KIND_IS_CODE( sib_id ))   return NULL;

    int index = GET_BOOK_CONTAINING_POINTEE( program_counter );

    while (!SIBID_ID_IS_BIGCHUNK_RECORD(sib_id)) {
	//
	sib_id =  book_to_sibid__global[ --index ];
    }

    Hugechunk_Region* region =  (Hugechunk_Region*)  ADDRESS_OF_BOOK( index );

    return get_codechunk_comment_string_else_null( get_hugechunk_holding_pointee( region, (Val)program_counter ) );
}



Unt8*   get_codechunk_comment_string_else_null   (Hugechunk* bdp) {
    //  ==================
    //
    // Return the tag of the given code chunk.

    Unt8* last_byte =  (Unt8*) bdp->chunk  +  bdp->bytesize -1;

    int   kx        =  *last_byte * WORD_BYTESIZE;

    return   (last_byte - kx) + 1;
}

// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.
