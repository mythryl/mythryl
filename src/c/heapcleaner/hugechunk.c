// hugechunk.c
//
// Code for managing hugechunk quires.

#include "../mythryl-config.h"

#include "runtime-base.h"
#include "get-quire-from-os.h"
#include "heap.h"
#include <string.h>

#ifdef BO_DEBUG


//
void         print_hugechunk_quire_map   (Hugechunk_Quire* hq)   {
    //       =========================
    //
    Hugechunk*	dp;
    Hugechunk*	dq;
    int		i;

    debug_say ("[%d] %d/%d, @%#x: ", hq->minGen, hq->nFree, hq->page_count, hq->first_ram_quantum);

    for (i = 0, dq = NULL;  i < hq->page_count;  i++) {

	dp = hq->hugechunk_page_to_hugechunk[i];

	if (dp != dq) {
	    debug_say ("|");
	    dq = dp;
	}

	if (HUGECHUNK_IS_FREE(dp))   debug_say ("_");
	else                         debug_say ("X");
    }
    debug_say ("|\n");
}

#endif

//
Hugechunk*   allocate_hugechunk_quire   (
    //       =========================
    // 
    Heap* heap,
    Punt  bytesize
){
    // Allocate a hugechunk quire that is
    // large enough to hold an chunk of at
    // least 'bytesize' bytes.
    //
    // It returns the descriptor for the
    // free hugechunk that is the quire.
    //
    // NOTE: We do not mark the book_to_sibid__global entries for the quire;
    //       this must be done by the caller.

    int npages;
    int old_npages;

    Punt  record_bytesize;
    Punt  quire_bytesize;

    Quire*  quire;

    // Compute the memory chunk size.
    // NOTE: there probably is a closed form for this,
    // but I'm too lazy to try to figure it out.        XXX SUCKO FIXME

    npages =    ROUND_UP_TO_POWER_OF_TWO (
		    //
		    bytesize,
		    HUGECHUNK_RAM_QUANTUM_IN_BYTES
		)
		>>
		LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES;

    do {
	old_npages = npages;

	record_bytesize = ROUND_UP_TO_POWER_OF_TWO(HUGECHUNK_QUIRE_RECORD_BYTESIZE(npages), HUGECHUNK_RAM_QUANTUM_IN_BYTES);

	bytesize = (npages << LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES);

	quire_bytesize = BOOKROUNDED_BYTESIZE( record_bytesize + bytesize );

	quire_bytesize =  (quire_bytesize < MINIMUM_HUGECHUNK_QUIRE_BYTESIZE)			// MINIMUM_HUGECHUNK_QUIRE_BYTESIZE	is from   src/c/h/heap.h
                       ?  MINIMUM_HUGECHUNK_QUIRE_BYTESIZE					// MINIMUM_HUGECHUNK_QUIRE_BYTESIZE was 128KB at last check.
                       :  quire_bytesize;

	npages = (quire_bytesize - record_bytesize) >> LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES;

    } while (npages != old_npages);

    quire =  obtain_quire_from_os(  quire_bytesize  );						if (!quire) die( "Unable to allocate hugechunk quire.");

    Hugechunk_Quire*
	//
        hq = (Hugechunk_Quire*) BASE_ADDRESS_OF_QUIRE( quire );					// BASE_ADDRESS_OF_QUIRE	is from   src/c/h/get-quire-from-os.h


    Hugechunk* chunk = MALLOC_CHUNK( Hugechunk );						if (!chunk)	 die( "Unable to allocate hugechunk descriptor.");


    // Initialize the hugechunk record:
    //
    hq->first_ram_quantum	=  (Punt) hq + record_bytesize;
    //
    hq->page_count	= npages;
    hq->free_pages	= npages;
    //
    hq->age_of_youngest_live_chunk_in_quire
	=
	MAX_AGEGROUPS;

    hq->quire			= quire;
    //
    hq->next			= heap->hugechunk_quires;
    heap->hugechunk_quires	= hq;

    heap->hugechunk_quire_count++;

    for (int i = 0;  i < npages;  i++) {
        //
	hq->hugechunk_page_to_hugechunk[i] = chunk;
    }

    // Initialize the descriptor for the hugechunk_quire's memory:
    //
    chunk->chunk	 	=  hq->first_ram_quantum;
    chunk->bytesize 		=  bytesize;
    //
    chunk->hugechunk_state	=  FREE_HUGECHUNK;
    chunk->hugechunk_quire 	=  hq;

    #ifdef BO_DEBUG
	debug_say ("allocate_hugechunk_quire: %d pages @ %#x\n", npages, hq->first_ram_quantum);
    #endif

    return chunk;
}


//
Hugechunk*   allocate_hugechunk   (
    //       ==================
    //
    Heap*     heap,
    int       age,
    //
    Punt    hugechunk_bytesize
) {
    // Allocate a hugechunk of the given size.

    Hugechunk*   dp;
    Hugechunk*   new_chunk;

    Hugechunk_Quire* hq;

    int first_ram_quantum;

    Punt total_bytesize
	=
	ROUND_UP_TO_POWER_OF_TWO( hugechunk_bytesize, HUGECHUNK_RAM_QUANTUM_IN_BYTES );

    int npages =  total_bytesize >> LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES;

    Hugechunk*
        //
        header = heap->hugechunk_freelist;

    // Search for a free chunk that is big enough (first-fit):
    //
    for (dp = header->next;
	 //
	 dp != header  &&  dp->bytesize < total_bytesize;
	 //
	 dp = dp->next
    );

    if (dp == header) {
	//
        // No free chunk fits, so allocate a new quire:
	//
	dp = allocate_hugechunk_quire( heap, total_bytesize );

	hq = dp->hugechunk_quire;

	if (dp->bytesize == total_bytesize) {
	    //
	    // Allocate the whole quire to the chunk:
	    //
	    new_chunk = dp;

	} else {

	    // Split the free chunk:
	    //
	    new_chunk			= MALLOC_CHUNK(Hugechunk);
	    new_chunk->chunk		= dp->chunk;
	    new_chunk->hugechunk_quire	= hq;

	    dp->chunk      =  (Punt)(dp->chunk) + total_bytesize;
	    dp->bytesize  -=  total_bytesize;

	    insert_hugechunk_in_doubly_linked_list( heap->hugechunk_freelist, dp );						// insert_hugechunk_in_doubly_linked_list	def in   src/c/h/heap.h

	    first_ram_quantum	= GET_HUGECHUNK_FOR_POINTER_PAGE(hq, new_chunk->chunk);

	    for (int i = 0;  i < npages;  i++) {
		//
		hq->hugechunk_page_to_hugechunk[ first_ram_quantum + i ]
		    =
		    new_chunk;
            }
	}

    } else if (dp->bytesize == total_bytesize) {

	remove_hugechunk_from_doubly_linked_list(dp);						// remove_hugechunk_from_doubly_linked_list	def in   src/c/h/heap.h
	new_chunk = dp;
	hq = dp->hugechunk_quire;

    } else {

        // Split the free chunk, leaving dp in the free list:
        //
	hq =  dp->hugechunk_quire;

	new_chunk		    = MALLOC_CHUNK( Hugechunk );
	new_chunk->chunk	    = dp->chunk;
	new_chunk->hugechunk_quire  = hq;

	dp->chunk     = (Punt)(dp->chunk) + total_bytesize;
	dp->bytesize -= total_bytesize;

	first_ram_quantum =  GET_HUGECHUNK_FOR_POINTER_PAGE(hq, new_chunk->chunk);

	for (int i = 0;  i < npages;  i++) {
	    //
	    dp->hugechunk_quire->hugechunk_page_to_hugechunk[ first_ram_quantum + i ]
		=
		new_chunk;
        }
    }

    new_chunk->bytesize        =  hugechunk_bytesize;
    new_chunk->hugechunk_state =  JUNIOR_HUGECHUNK;
    new_chunk->age	       =  age;

    hq->free_pages  -=  npages;

    if (hq->age_of_youngest_live_chunk_in_quire > age) {
	hq->age_of_youngest_live_chunk_in_quire = age;
	//
	set_book2sibid_entries_for_range (book_to_sibid__global,  (Val*)hq,  BYTESIZE_OF_QUIRE( hq->quire ),  HUGECHUNK_DATA_SIBID( age ));

	book_to_sibid__global[ GET_BOOK_CONTAINING_POINTEE( hq ) ]
	    =
	    HUGECHUNK_RECORD_SIBID( age );
    }

    #ifdef BO_DEBUG
        debug_say ("allocate_hugechunk: %d bytes @ %#x\n", hugechunk_bytesize, new_chunk->chunk);
        print_hugechunk_quire_map(hq);
    #endif

    return new_chunk;
}

//
void         free_hugechunk   (
    //       ==============
    //
    Heap*     heap,
    Hugechunk* chunk
){
    // Mark a big chunk as free and add it to the free list.

    Hugechunk_Quire* hq =  chunk->hugechunk_quire;

    Hugechunk* dp;

    Punt  total_bytesize
	=
	ROUND_UP_TO_POWER_OF_TWO( chunk->bytesize, HUGECHUNK_RAM_QUANTUM_IN_BYTES);

    int first_ram_quantum =  GET_HUGECHUNK_FOR_POINTER_PAGE(hq, chunk->chunk);
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
	print_hugechunk_quire_map(hq);
    #endif

    if (first_ram_quantum > 0
    &&  HUGECHUNK_IS_FREE( hq->hugechunk_page_to_hugechunk[ first_ram_quantum -1 ])
    ){
        // Coalesce with adjacent free chunk:
        //
	dp = hq->hugechunk_page_to_hugechunk[ first_ram_quantum -1 ];
	//
	remove_hugechunk_from_doubly_linked_list( dp );						// remove_hugechunk_from_doubly_linked_list	def in   src/c/h/heap.h

	for (int i = GET_HUGECHUNK_FOR_POINTER_PAGE(hq, dp->chunk);   i < first_ram_quantum;   i++) {
	    //
	    hq->hugechunk_page_to_hugechunk[i] = chunk;
        }

	chunk->chunk = dp->chunk;

	total_bytesize += dp->bytesize;

	FREE( dp );
    }

    if (last_ram_quantum < hq->page_count
    &&  HUGECHUNK_IS_FREE( hq->hugechunk_page_to_hugechunk[ last_ram_quantum ])
    ){
	//
        // Coalesce with adjacent free chunk:

	dp = hq->hugechunk_page_to_hugechunk[ last_ram_quantum ];

	remove_hugechunk_from_doubly_linked_list( dp );

	for (int i = last_ram_quantum, j = i+(dp->bytesize >> LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES);
             i < j;
             i++
        ){
	    //
	    hq->hugechunk_page_to_hugechunk[ i ]
		=
		chunk;
        }

	total_bytesize += dp->bytesize;
	FREE (dp);
    }

    chunk->bytesize   =  total_bytesize;
    chunk->hugechunk_state =  FREE_HUGECHUNK;

    hq->free_pages +=   (last_ram_quantum - first_ram_quantum);

    // What if (hq->free_pages == hq->page_count) ??? XXX BUGGO FIXME

    // Add chunk to freelist:
    //
    insert_hugechunk_in_doubly_linked_list( heap->hugechunk_freelist, chunk );						// insert_hugechunk_in_doubly_linked_list	def in   src/c/h/heap.h
}

//
Hugechunk*   address_to_hugechunk   (Val addr) {
    //       ====================
    //
    // Given an address within a hugechunk,
    // return the chunk's descriptor record.

    Sibid*    book2sibid = book_to_sibid__global;
    Sibid    sibid;

    int  i;
    for (i = GET_BOOK_CONTAINING_POINTEE(addr);  !SIBID_ID_IS_BIGCHUNK_RECORD( sibid = book2sibid[ i ]);  i--);

    Hugechunk_Quire*
	//
	hq =  (Hugechunk_Quire*) ADDRESS_OF_BOOK( i );

    return get_hugechunk_holding_pointee( hq, addr );
}


//
Unt8*        codechunk_comment_string_for_program_counter   (Val_Sized_Unt  program_counter)   {
    //       ============================================
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

    Hugechunk_Quire*
	//
	hq =  (Hugechunk_Quire*)  ADDRESS_OF_BOOK( index );

    return get_codechunk_comment_string_else_null( get_hugechunk_holding_pointee( hq, (Val)program_counter ) );
}


//
Unt8*        get_codechunk_comment_string_else_null   (Hugechunk* bdp) {
    //       ======================================
    //
    // Return the tag of the given code chunk.

    Unt8* last_byte =  (Unt8*) bdp->chunk  +  bdp->bytesize -1;

    int   kx        =  *last_byte * WORD_BYTESIZE;

    return   (last_byte - kx) + 1;
}

// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.





/*
##########################################################################
#   The following is support for outline-minor-mode in emacs.		 #
#  ^C @ ^T hides all Text. (Leaves all headings.)			 #
#  ^C @ ^A shows All of file.						 #
#  ^C @ ^Q Quickfolds entire file. (Leaves only top-level headings.)	 #
#  ^C @ ^I shows Immediate children of node.				 #
#  ^C @ ^S Shows all of a node.						 #
#  ^C @ ^D hiDes all of a node.						 #
#  ^HFoutline-mode gives more details.					 #
#  (Or do ^HI and read emacs:outline mode.)				 #
#									 #
# Local variables:							 #
# mode: outline-minor							 #
# outline-regexp: "[A-Za-z]"			 		 	 #
# End:									 #
##########################################################################
*/
