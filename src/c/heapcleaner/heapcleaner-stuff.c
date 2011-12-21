// heapcleaner-stuff.c
//
// Garbage collection utility routines.

#include "../mythryl-config.h"

#include <stdarg.h>
#include <string.h>
#include "runtime-base.h"
#include "runtime-configuration.h"
#include "runtime-values.h"
#include "get-multipage-ram-region-from-os.h"
#include "coarse-inter-agegroup-pointers-map.h"
#include "heap.h"


// Debug routine added while chasing a Heisenbug that strikes
// in forward_agegroup0_chunk_to_agegroup_1(), in the hope of
// adding some useful context for the reported chunk address:
//
void log_task( Task* task ) {
    //
    log_if("log_task:                       task x=%p",  task);
    log_if("log_task:                       heap x=%x",  task->heap);
    log_if("log_task:                    pthread x=%x",  task->pthread);
    log_if("log_task:                pthread->id x=%x",  (unsigned int)(task->pthread->tid));
    log_if("log_task:    heap_allocation_pointer x=%x",  task->heap_allocation_pointer);
    log_if("log_task:      heap_allocation_limit x=%x",  task->heap_allocation_limit);
    log_if("log_task: real_heap_allocation_limit x=%x",  task->real_heap_allocation_limit);
    log_if("log_task:                   argument x=%x",  task->argument);
    log_if("log_task:                       fate x=%x",  task->fate);
    log_if("log_task:            current_closure x=%x",  task->current_closure);
    log_if("log_task:              link_register x=%x",  task->link_register);
    log_if("log_task:            program_counter x=%x",  task->program_counter);
    log_if("log_task:             exception_fate x=%x",  task->exception_fate);
    log_if("log_task:             current_thread x=%x",  task->current_thread);
    log_if("log_task:             heap_changelog x=%x",  task->heap_changelog);
    log_if("log_task:            fault_exception x=%x",  task->fault_exception);
    log_if("log_task:   faulting_program_counter x=%x",  task->faulting_program_counter);
    log_if("log_task:            protected_c_arg x=%x",  task->protected_c_arg);
    log_if("log_task:                  &heapvoid x=%x", &task->heapvoid);
    log_if("log_task: Following stuff is in task->struct");
    log_if("log_task:           agegroup0_buffer x=%x",  task->heap->agegroup0_buffer);
    log_if("log_task:  agegroup0_buffer_bytesize x=%x",  task->heap->agegroup0_buffer_bytesize);
    log_if("log_task:           sum of above two x=%x",  (char*)(task->heap->agegroup0_buffer) + task->heap->agegroup0_buffer_bytesize);
    log_if("log_task:       multipage_ram_region x=%x",  task->heap->multipage_ram_region);
    log_if("log_task:           active_agegroups d=%d",  task->heap->active_agegroups);
    log_if("log_task: oldest_agegroup_keeping_idle_fromspace_buffers d=%d",  task->heap->oldest_agegroup_keeping_idle_fromspace_buffers);
    log_if("log_task:  hugechunk_ramregion_count d=%d",  task->heap->hugechunk_ramregion_count);
    log_if("log_task:      total_bytes_allocated x=(%x,%x) (millions, 1s)",  task->heap->total_bytes_allocated.millions, task->heap->total_bytes_allocated.ones );

    for (int i = 0; i < task->heap->active_agegroups; ++i) {
	//
	Agegroup* a = task->heap->agegroup[ i ];
	log_if("log_task:           agegroup[%d] x=%x (holds agegroup %d)",  i, a, i+1);
	log_if("log_task:           a->age       d=%d",  a->age);
	log_if("log_task:           a->cleanings d=%d",  a->cleanings);
	log_if("log_task:           a->ratio     x=%x (Desired number of collections of the previous agegroup for one collection of this agegroup)",  a->ratio);
	log_if("log_task:           a->last_cleaning_count_of_younger_agegroup d=%d",  a->last_cleaning_count_of_younger_agegroup);

	for (int s = 0; s < MAX_PLAIN_ILKS; ++s) {
	    //
	    log_if("log_task:");
	    log_if("log_task:          a->sib[%d]    x=%x",  s, a->sib[s]);
	    log_if("log_task:          a->sib[%d].id d=%d",  s, a->sib[s]->id);
	    log_if("log_task:");
	    log_if("log_task:          a->sib[%d].tospace            x=%x",  s, a->sib[s]->tospace);
	    log_if("log_task:          a->sib[%d].tospace_bytesize   x=%x",  s, a->sib[s]->tospace_bytesize);
	    log_if("log_task:          a->sib[%d].tospace_limit      x=%x",  s, a->sib[s]->tospace_limit);
	    log_if("log_task:");
	    log_if("log_task:          a->sib[%d].fromspace          x=%x",  s, a->sib[s]->fromspace);
	    log_if("log_task:          a->sib[%d].fromspace_bytesize x=%x",  s, a->sib[s]->fromspace_bytesize);
	    log_if("log_task:          a->sib[%d].fromspace_used_end x=%x",  s, a->sib[s]->fromspace_used_end);
	    log_if("log_task:");
	    log_if("log_task:          a->sib[%d].next_tospace_word_to_allocate x=%x",  s, a->sib[s]->next_tospace_word_to_allocate);
	    log_if("log_task:          a->sib[%d].next_word_to_sweep_in_tospace x=%x",  s, a->sib[s]->next_word_to_sweep_in_tospace);
	    log_if("log_task:          a->sib[%d].repairlist x=%x",  s, a->sib[s]->repairlist);
	    log_if("log_task:          a->sib[%d].end_of_fromespace_oldstuff x=%x",  s, a->sib[s]->end_of_fromspace_oldstuff);
	}
    }
}



void   zero_agegroup0_overrun_tripwire_buffer( Task* task ) {
    // ==========================================
    //
    // To detect allocation buffer overrun, we maintain
    // an always-all-zeros buffer of AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS
    // Val_Sized_Ints at the end of each agegroup0 buffer.
    // Here we zero that out:
    //
    Val_Sized_Int* p = (Val_Sized_Int*) (((char*)(task->real_heap_allocation_limit)) + MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER);
    //
    for (int i = 0;
             i < AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS;
             ++i
    ){
	//
	p[i] = 0;
    }
//  log_if("zero_agegroup0_overrun_tripwire_buffer: Done zeroing %x -> %x", p, p+(AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS-1));	// Commented out because it spams the logfile with gigabytes of text.
}

static char*  val_sized_unt_as_ascii(  char* buf,  Val_Sized_Unt u ) {
    //        ======================
    //
    char* p = buf;
    //
    for (int i = 0;  i < sizeof(Val_Sized_Unt);  ++i) {
	//
	char c =  u & 0xFF;
	u      =  u >> 8;
	*p++ = (c >= ' ' && c <= '~') ? c : '.';
    } 

    *p++ = '\0';

    return buf;
}

void   check_agegroup0_overrun_tripwire_buffer( Task* task, char* caller ) {
    // ==========================================
    //
    // To detect allocation buffer overrun, we maintain
    // an always-all-zeros buffer of AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS
    // Val_Sized_Ints at the end of each agegroup0 buffer.
    // Here we verify that it is all zeros:
    //
#ifndef SOON
    Val_Sized_Int* p = (Val_Sized_Int*) (((char*)(task->real_heap_allocation_limit)) + MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER);
    //
    for (int i = AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS; i --> 0; ) {
	//
	if (p[i] != 0) {
	    //
	    log_if("check_agegroup0_overrun_tripwire_buffer:  While checking %x -> %x agegroup0 buffer overrun of %d words detected at %s", p, p+(AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_WORDS-1), i, caller);
	    //
	    for (int j = 0;   j <= i;   ++j) {
		//
		char buf[ 132 ];
		log_if("check_agegroup0_overrun_tripwire_buffer: tripwire_buffer[%3d] x=%08x s='%s'", j, p[j], val_sized_unt_as_ascii(buf, (Val_Sized_Unt)(p[j])));
	    }
	    die( "check_agegroup0_overrun_tripwire_buffer:  Agegroup0 buffer overrun");
	    exit(1);										// die() should never return, so this should never execute. But gcc understands it better.
	}
    }
#endif
}

Status   allocate_and_partition_an_agegroup   (Agegroup* ag) {
    //   ==================================
    // 
    // Allocate and partition the space for an agegroup.

    Multipage_Ram_Region*	heapchunk;

    Sib*  ap;

    // Compute the total size:
    //
    Punt  tot_size =  0;
    //
    for (int i = 0;  i < MAX_PLAIN_ILKS;  i++) {
	//
	if (sib_is_active( ag->sib[ i ] )) {							// sib_is_active	def in    src/c/h/heap.h
	    //
	    tot_size +=  ag->sib[ i ]->tospace_bytesize;
	}
    }

    if (ag->saved_fromspace_ram_region != NULL
    && BYTESIZE_OF_MULTIPAGE_RAM_REGION( ag->saved_fromspace_ram_region ) >= tot_size
    ){
	heapchunk =  ag->saved_fromspace_ram_region;

	ag->saved_fromspace_ram_region =  NULL;

    } else if ((heapchunk = obtain_multipage_ram_region_from_os( tot_size )) == NULL) {
	//
	// Eventually we should try to allocate the agegroup
	//as separate chunks instead of failing.			XXX BUGGO FIXME
	//
	return FAILURE;
    }

    // Initialize the chunks:
    //
    ag->tospace_ram_region = heapchunk;
    //
    #ifdef VERBOSE
        debug_say ("allocate_and_partition_an_agegroup[%d]: tot_size = %d, [%#x, %#x)\n",
            ag->age,
            tot_size,
            BASE_ADDRESS_OF_MULTIPAGE_RAM_REGION( heapchunk ),
            BASE_ADDRESS_OF_MULTIPAGE_RAM_REGION( heapchunk ) + BYTESIZE_OF_MULTIPAGE_RAM_REGION( heapchunk )
        );
    #endif
    //
    Val* p =  (Val*) BASE_ADDRESS_OF_MULTIPAGE_RAM_REGION( heapchunk );
    //
    for (int i = 0;  i < MAX_PLAIN_ILKS;  i++) {
        //
	ap = ag->sib[ i ];
        //
	if (!sib_is_active(ap)) {							// sib_is_active	def in    src/c/h/heap.h
            //
	    ap->tospace	= NULL;
	    ap->next_tospace_word_to_allocate		= NULL;
	    ap->next_word_to_sweep_in_tospace	= NULL;
	    ap->tospace_limit		= NULL;
            //
	} else {
            //
	    ap->tospace	= p;
	    ap->next_tospace_word_to_allocate		= p;
	    ap->next_word_to_sweep_in_tospace	= p;
            //
	    p = (Val*)((Punt)p + ap->tospace_bytesize);
	    ap->tospace_limit	= p;
	    set_book2sibid_entries_for_range( book_to_sibid__global, ap->tospace, ap->tospace_bytesize, ap->id );

	    #ifdef VERBOSE
	        debug_say ("  %#x:  [%#x, %#x)\n", ap->id, ap->next_tospace_word_to_allocate, p);
	    #endif
	}
    }

    ap = ag->sib[ PAIR_ILK ];

    if (sib_is_active(ap)) {
        //
        // The first slot of pair-space must not be used,
        // else poly-equal might fault:
        //
	*(ap->next_tospace_word_to_allocate++) = HEAP_VOID;
	*(ap->next_tospace_word_to_allocate++) = HEAP_VOID;
	ap->tospace = ap->next_tospace_word_to_allocate;
	ap->tospace_bytesize -= (2*WORD_BYTESIZE);
	ap->next_word_to_sweep_in_tospace = ap->next_tospace_word_to_allocate;
    }   

    return SUCCESS;
}								// fun allocate_and_partition_an_agegroup



void   free_agegroup   (Heap* heap,  int g) {
    // ====================
    //
    Agegroup*  ag = heap->agegroup[ g ];

    if (ag->fromspace_ram_region == NULL)   return;

    #ifdef VERBOSE
	debug_say ("free_agegroup [%d]: [%#x, %#x)\n",
            g+1,
            BASE_ADDRESS_OF_MULTIPAGE_RAM_REGION( ag->fromspace_ram_region ),
            BASE_ADDRESS_OF_MULTIPAGE_RAM_REGION( ag->fromspace_ram_region ) + BYTESIZE_OF_MULTIPAGE_RAM_REGION( ag->fromspace_ram_region )
        );
    #endif


    if (g >= heap->oldest_agegroup_keeping_idle_fromspace_buffers) {
        //
	return_multipage_ram_region_to_os( ag->fromspace_ram_region );
	//
    } else {
        //
	if (ag->saved_fromspace_ram_region == NULL) {
	    ag->saved_fromspace_ram_region =  ag->fromspace_ram_region;
	} else {

	    if (BYTESIZE_OF_MULTIPAGE_RAM_REGION( ag->saved_fromspace_ram_region )
              > BYTESIZE_OF_MULTIPAGE_RAM_REGION( ag->fromspace_ram_region       )
            ){
	        //
		return_multipage_ram_region_to_os( ag->fromspace_ram_region );
	    } else {
		return_multipage_ram_region_to_os( ag->saved_fromspace_ram_region );
		ag->saved_fromspace_ram_region = ag->fromspace_ram_region;
	    }
	}
    }

    // NOTE: Since the sib buffers are contiguous,
    // we could do this in one call:
    //
    ag->fromspace_ram_region = NULL;
    //	
    for (int i = 0;  i < MAX_PLAIN_ILKS;  i++) {
	//
	Sib* ap =  ag->sib[ i ];

	if (ap->fromspace != NULL) {
	    //
	    set_book2sibid_entries_for_range (book_to_sibid__global, ap->fromspace, ap->fromspace_bytesize, UNMAPPED_BOOK_SIBID);

	    ap->fromspace = NULL;
	    ap->fromspace_bytesize = 0;
	    ap->fromspace_used_end = NULL;
	}
    }
}								// fun free_agegroup


void   make_new_coarse_inter_agegroup_pointers_map_for_agegroup   (Agegroup* ag) {
    // ========================================================
    // 
    // Bind in a new coarse_inter_agegroup_pointers_map
    // vector for the given agegroup VECTOR_ILK,
    // reclaiming the old map.

    Sib* ap =  ag->sib[ VECTOR_ILK ];												// We only need these maps for the VECTOR_ILK sibs.

    int  map_size_in_slots
	=
	ap->tospace_bytesize / CARD_BYTESIZE;

    int  agegroup0_buffer_bytesize = CARDMAP_BYTESIZE( map_size_in_slots );

    if (ag->coarse_inter_agegroup_pointers_map != NULL) {
        //
	FREE( ag->coarse_inter_agegroup_pointers_map );
    }
    ag->coarse_inter_agegroup_pointers_map =   (Coarse_Inter_Agegroup_Pointers_Map*)   MALLOC( agegroup0_buffer_bytesize );

    ag->coarse_inter_agegroup_pointers_map->map_bytesize =   agegroup0_buffer_bytesize;

    if (ag->coarse_inter_agegroup_pointers_map == NULL) 	die ("unable to malloc coarse_inter_agegroup_pointers_map vector");

    ag->coarse_inter_agegroup_pointers_map->base_address =  ap->tospace;
    ag->coarse_inter_agegroup_pointers_map->card_count   =  map_size_in_slots;

    memset(
        ag->coarse_inter_agegroup_pointers_map->min_age,
        CLEAN_CARD,
        agegroup0_buffer_bytesize -  (sizeof( Coarse_Inter_Agegroup_Pointers_Map ) - WORD_BYTESIZE)
    );
}


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


void   null_out_newly_dead_weak_pointers   (Heap* heap) {
    // ===========================
    // 
    // Weak pointers are not followed by the cleaner (garbage collector).
    // The point of this is to allow us to (for example) maintain an index
    // of all existing items of some kind while still allowing them to be
    // garbage collected when no longer needed.  The index will see the weak
    // pointers to recycled values turn Void at that point.  Implementing
    // that is our job here.
    // 
    // Scan the list of weak pointers,
    // nullifying those that refer to
    // dead (i.e., from-space) chunks.

    // This list gets constructed in the functions
    // 
    //     forward_special_chunk()
    // in
    //     src/c/heapcleaner/heapclean-agegroup0.c
    //     src/c/heapcleaner/heapclean-n-agegroups.c

    if (heap->weak_pointers_forwarded_during_cleaning == NULL)   return;			// No work to do.

    Sibid*	   b2s    =  book_to_sibid__global;						// Cache global locally for speed.   book_to_sibid__global	def in    src/c/heapcleaner/heapcleaner-initialization.c

    Val* next;

												// debug_say ("scan_weak_pointers:\n");

    for (Val* p = heap->weak_pointers_forwarded_during_cleaning;
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
	case RECORD_KIND:
	case STRING_KIND:
	case VECTOR_KIND:
	    {
		Val tagword = chunk[-1];

		if (tagword == FORWARDED_CHUNK_TAGWORD) {
		    //
		    p[0] = WEAK_POINTER_TAGWORD;
		    p[1] = PTR_CAST( Val, FOLLOW_FWDCHUNK(chunk));
		    // debug_say ("forwarded to %#x\n", FOLLOW_FWDCHUNK(chunk));

		} else {

		    p[0] = NULLED_WEAK_POINTER_TAGWORD;
		    p[1] = HEAP_VOID;

		    // debug_say ("nullified\n");
		}
	    }
	    break;

	case PAIR_KIND:
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
		    p[0] = WEAK_POINTER_TAGWORD;
		    p[1] = PTR_CAST( Val,  FOLLOW_PAIRSPACE_FORWARDING_POINTER( tagword, chunk ) );

		    // debug_say ("(pair) forwarded to %#x\n", FOLLOW_PAIRSPACE_FORWARDING_POINTER( tagword, chunk ));

		} else {

		    p[0] = NULLED_WEAK_POINTER_TAGWORD;
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

    heap->weak_pointers_forwarded_during_cleaning
	=
	NULL;
}



// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

