// heapcleaner-stuff.c
//
// Garbage collection utility routines.

#include "../mythryl-config.h"

#include <stdarg.h>
#include <string.h>
#include "runtime-base.h"
#include "runtime-configuration.h"
#include "runtime-values.h"
#include "get-quire-from-os.h"
#include "coarse-inter-agegroup-pointers-map.h"
#include "heap.h"

//
Status   set_up_tospace_sib_buffers_for_agegroup   (Agegroup* ag) {
    //   =======================================
    // 
    // Allocate and partition the space for an agegroup.
    //
    // This fn is called from:
    //
    //     src/c/heapcleaner/heapcleaner-initialization.c
    //     src/c/heapcleaner/import-heap.c
    //     src/c/heapcleaner/datastructure-pickler-cleaner.c
    //     src/c/heapcleaner/heapclean-n-agegroups.c

    Quire* quire;
    Sib*   sib;

    // Compute the total size:
    //
    Punt  total_bytes =  0;
    //
    for (int i = 0;  i < MAX_PLAIN_SIBS;  i++) {
	//
	if (sib_is_active( ag->sib[ i ] )) {							// sib_is_active	def in    src/c/h/heap.h
	    //
	    total_bytes +=  ag->sib[ i ]->tospace.bytesize;
	}
    }

    if (ag->retained_fromspace_quire != NULL
    && BYTESIZE_OF_QUIRE( ag->retained_fromspace_quire ) >= total_bytes
    ){
	quire =  ag->retained_fromspace_quire;

	ag->retained_fromspace_quire =  NULL;

    } else if ((quire = obtain_quire_from_os( total_bytes )) == NULL) {
	//
	// Eventually we should try to allocate the agegroup
	//as separate chunks instead of failing.			XXX SUCKO FIXME
	//
	return FALSE;
    }

    // Initialize the individual sib buffers:
    //
    ag->tospace_quire = quire;
    //
    #ifdef VERBOSE
        debug_say ("set_up_tospace_sib_buffers_for_agegroup[%d]: total_bytes = %d, [%#x, %#x)\n",
            ag->age,
            total_bytes,
            BASE_ADDRESS_OF_QUIRE( quire ),						// BASE_ADDRESS_OF_QUIRE	is from   src/c/h/get-quire-from-os.h
            BASE_ADDRESS_OF_QUIRE( quire ) + BYTESIZE_OF_QUIRE( quire )
        );
    #endif
    //
    Val* p =  (Val*) BASE_ADDRESS_OF_QUIRE( quire );
    //
    for (int i = 0;  i < MAX_PLAIN_SIBS;  i++) {
        //
	sib = ag->sib[ i ];
        //
	if (!sib_is_active(sib)) {							// sib_is_active	def in    src/c/h/heap.h
            //
	    sib->tospace.start		= NULL;
	    sib->tospace.used_end	= NULL;
	    sib->tospace.swept_end	= NULL;
	    sib->tospace.limit		= NULL;
            //
	} else {
            //
	    sib->tospace.start		= p;
	    sib->tospace.used_end	= p;
	    sib->tospace.swept_end	= p;
            //
	    p = (Val*)((Punt)p + sib->tospace.bytesize);
	    sib->tospace.limit	= p;
	    set_book2sibid_entries_for_range( book_to_sibid__global, sib->tospace.start, sib->tospace.bytesize, sib->id );

	    #ifdef VERBOSE
	        debug_say ("  %#x:  [%#x, %#x)\n", sib->id, sib->tospace.used_end, p);
	    #endif
	}
    }

    sib = ag->sib[ RO_CONSCELL_SIB ];

    if (sib_is_active(sib)) {
        //
        // The first slot of pair-space must not be used,
        // else poly-equal might fault:
        //
	*(sib->tospace.used_end++) = HEAP_VOID;
	*(sib->tospace.used_end++) = HEAP_VOID;
        //
	sib->tospace.start	 = sib->tospace.used_end;
	sib->tospace.bytesize	-= (2*WORD_BYTESIZE);
	sib->tospace.swept_end	 = sib->tospace.used_end;
    }   

    return TRUE;
}								// fun set_up_tospace_sib_buffers_for_agegroup


//
void   free_agegroup   (Heap* heap,  int g) {
    // =============
    //
    Agegroup*  ag = heap->agegroup[ g ];

    if (ag->fromspace_quire == NULL)   return;

    #ifdef VERBOSE
	debug_say ("free_agegroup [%d]: [%#x, %#x)\n",
            g+1,
            BASE_ADDRESS_OF_QUIRE( ag->fromspace_quire ),
            BASE_ADDRESS_OF_QUIRE( ag->fromspace_quire ) + BYTESIZE_OF_QUIRE( ag->fromspace_quire )
        );
    #endif


    if (g >= heap->oldest_agegroup_retaining_fromspace_sibs_between_heapcleanings) {
        //
	return_quire_to_os( ag->fromspace_quire );
	//
    } else {
        //
	if (ag->retained_fromspace_quire == NULL) {
	    ag->retained_fromspace_quire =  ag->fromspace_quire;
	} else {

	    if (BYTESIZE_OF_QUIRE( ag->retained_fromspace_quire )
              > BYTESIZE_OF_QUIRE( ag->fromspace_quire       )
            ){
	        //
		return_quire_to_os( ag->fromspace_quire );
	    } else {
		return_quire_to_os( ag->retained_fromspace_quire );
		ag->retained_fromspace_quire = ag->fromspace_quire;
	    }
	}
    }

    // NOTE: Since the sib buffers are contiguous,
    // we could do this in one call:
    //
    ag->fromspace_quire = NULL;
    //	
    for (int i = 0;  i < MAX_PLAIN_SIBS;  i++) {
	//
	Sib* ap =  ag->sib[ i ];

	if (ap->fromspace.start != NULL) {
	    //
	    set_book2sibid_entries_for_range (book_to_sibid__global, ap->fromspace.start, ap->fromspace.bytesize, UNMAPPED_BOOK_SIBID);
	    //
	    ap->fromspace.start	    = NULL;
	    ap->fromspace.bytesize  = 0;
	    ap->fromspace.used_end  = NULL;
	}
    }
}								// fun free_agegroup

//
void   make_new_coarse_inter_agegroup_pointers_map_for_agegroup   (Agegroup* ag) {
    // ========================================================
    // 
    // Bind in a new coarse_inter_agegroup_pointers_map
    // vector for the given agegroup RW_POINTERS_SIB,
    // reclaiming the old map.

    Sib* ap =  ag->sib[ RW_POINTERS_SIB ];												// We only need these maps for the RW_POINTERS_SIB sibs.

    int  map_size_in_slots
	=
	ap->tospace.bytesize / CARD_BYTESIZE;

    int  map_bytesize = CARDMAP_BYTESIZE( map_size_in_slots );

    if (ag->coarse_inter_agegroup_pointers_map != NULL) {
        //
	FREE( ag->coarse_inter_agegroup_pointers_map );
    }
    ag->coarse_inter_agegroup_pointers_map =   (Coarse_Inter_Agegroup_Pointers_Map*)   MALLOC( map_bytesize );

    ag->coarse_inter_agegroup_pointers_map->map_bytesize =   map_bytesize;

    if (ag->coarse_inter_agegroup_pointers_map == NULL) 	die ("unable to malloc coarse_inter_agegroup_pointers_map vector");

    ag->coarse_inter_agegroup_pointers_map->base_address =  ap->tospace.start;
    ag->coarse_inter_agegroup_pointers_map->card_count   =  map_size_in_slots;

    memset(
        ag->coarse_inter_agegroup_pointers_map->min_age,
        CLEAN_CARD,
        map_bytesize -  (sizeof( Coarse_Inter_Agegroup_Pointers_Map ) - WORD_BYTESIZE)
    );
}

//
void   set_book2sibid_entries_for_range   (Sibid* book2sibid,  Val* base_address,  Val_Sized_Unt bytesize,  Sibid sibid) {
    // =================================
    //
    // Mark the book_to_sibid__global entries corresponding to the range [ base_address, base_address+bytesize )
    // with sibid.

    #ifdef TWO_LEVEL_MAP
        #error two level map not supported
    #else
	int start =  GET_BOOK_CONTAINING_POINTEE( base_address );
	int end   =  GET_BOOK_CONTAINING_POINTEE( ((Punt)base_address) + bytesize );

	#ifdef VERBOSE
	    // debug_say("set_book2sibid_entries_for_range [%#x..%#x) as %#x\n", base_address, ((Punt)base_address)+bytesize, sibid);
	#endif

	while (start < end) {
	    //
	    book2sibid[ start++ ] =  sibid;
	}
    #endif
}

//
void   null_out_newly_dead_weakrefs   (Heap* heap) {
    // ============================
    // 
    // Weakrefs are special refcells not followed by the heapcleaner
    // (garbage collector). The point of this is to allow us to
    // (for example) maintain an index of all existing items of
    // some kind while still allowing them to be garbage collected
    // when no longer needed.  The index will see the weakrefs to
    // recycled values turn Void at that point.  Implementing that
    // is our job here.
    // 
    // Our input is a list of forwarded weakrefs.
    // This list gets constructed in the functions
    // 
    //     forward_special_chunk()
    // in
    //     src/c/heapcleaner/heapclean-agegroup0.c
    //     src/c/heapcleaner/heapclean-n-agegroups.c
    // 
    // We the list of weakrefs nulling out those that
    // refer to dead (i.e., from-space) chunks.

    if (heap->weakrefs_forwarded_during_heapcleaning == NULL)   return;			// No work to do.

    Sibid*	   b2s    =  book_to_sibid__global;						// Cache global locally for speed.   book_to_sibid__global	def in    src/c/heapcleaner/heapcleaner-initialization.c

    Val* next;

												// debug_say ("scan_weak_pointers:\n");

    for (Val* p = heap->weakrefs_forwarded_during_heapcleaning;
         p != NULL;
         p = next
    ){
	next       = PTR_CAST( Val*, UNMARK_POINTER( p[0] ));
	Val* chunk = (Val*) (Punt)   UNMARK_POINTER( p[1] );

												// debug_say ("  %#x --> %#x ", p+1, chunk);

	//
	switch (GET_KIND_FROM_SIBID( SIBID_FOR_POINTER(b2s, chunk ))) {
	    //
	case NEW_KIND:
	case RO_POINTERS_KIND:
	case NONPTR_DATA_KIND:
	case RW_POINTERS_KIND:
	    {
		Val tagword = chunk[-1];

		if (tagword == FORWARDED_CHUNK_TAGWORD) {
		    //
		    p[0] = WEAKREF_TAGWORD;
		    p[1] = PTR_CAST( Val, FOLLOW_FORWARDING_POINTER(chunk));
		    // debug_say ("forwarded to %#x\n", FOLLOW_FORWARDING_POINTER(chunk));

		} else {

		    p[0] = NULLED_WEAKREF_TAGWORD;
		    p[1] = HEAP_VOID;

		    // debug_say ("nullified\n");
		}
	    }
	    break;

	case RO_CONSCELL_KIND:
	    //
	    // Pairs are special because they don't have a tagword.
	    // (To save space.)
	    // Since they don't we mark forwarded pairs not by
	    // changing a tagword to FORWARDED_CHUNK_TAGWORD as usual,
	    // but rather by setting the TAGWORD_ATAG bit (0x2) on the
	    // first word in the Pair:
	    //
	    {   Val tagword;

		if (IS_TAGWORD( tagword = chunk[0] )) {
		    //
		    p[0] = WEAKREF_TAGWORD;
		    p[1] = PTR_CAST( Val,  FOLLOW_PAIRSPACE_FORWARDING_POINTER( tagword, chunk ) );

		    // debug_say ("(pair) forwarded to %#x\n", FOLLOW_PAIRSPACE_FORWARDING_POINTER( tagword, chunk ));

		} else {

		    p[0] = NULLED_WEAKREF_TAGWORD;
		    p[1] = HEAP_VOID;

		    // debug_say ("(pair) nullified\n");
		}
	    }
	    break;

	case CODE_KIND:
	    die ("weak big chunk");
	    break;
	}
    }

    heap->weakrefs_forwarded_during_heapcleaning =   NULL;
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
