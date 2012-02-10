// heapclean-n-agegroups.c
//
// This is the regular heapcleaner ("garbage collector"),
// for cleaning all agegroups *except* agegroup0.
//
// Agegroup0 is cleaned by a separate module
//     src/c/heapcleaner/heapclean-agegroup0.c
// because it has a totally different structure.)
//
// For a background discussion see:
//
//     src/A.GARBAGE-COLLECTOR.OVERVIEW
//

/*
###                        "Youth had been a habit of hers for so long,
###                         that she could not part with it."
###
###                                             -- Rudyard Kipling
*/

/*
###                        "It is best to do things systematically,
###                         since we are only humans, and disorder
###                         is our worst enemy."
###
###                                             -- Hesiod (c 800 - 720 BCE)
*/

/*
Includes:
*/
#if NEED_HEAPCLEANER_PAUSE_STATISTICS		// Cleaner pause statistics are UNIX dependent.
    #include "system-dependent-unix-stuff.h"
#endif

#include "../mythryl-config.h"

#include <stdio.h>

#include "runtime-base.h"
#include "runtime-configuration.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "get-quire-from-os.h"
#include "coarse-inter-agegroup-pointers-map.h"
#include "heap.h"
#include "heap-tags.h"
#include "copy-loop.h"
#include "runtime-timer.h"
#include "heapcleaner-statistics.h"

/*
Cleaner statistics stuff:
*/
 long	update_count__global		= 0;	// Referenced only in src/c/heapcleaner/heapclean-agegroup0.c
 long	total_bytes_allocated__global	= 0;	// Referenced only in src/c/heapcleaner/heapclean-agegroup0.c
 long	total_bytes_copied__global	= 0;	// Referenced only in src/c/heapcleaner/heapclean-agegroup0.c


#if NEED_HUGECHUNK_REFERENCE_STATISTICS		// "NEED_HUGECHUNK_REFERENCE_STATISTICS" does not appear outside this file, except for its definition in   src/c/mythryl-config.h
    //
    static long hugechunks_seen_count__local;
    static long hugechunk_lookups_count__local;
    static long hugechunks_forwarded_count__local;
    //
    #define COUNT_CODECHUNKS(sibid)	        {if (SIBID_KIND_IS_CODE(sibid))   ++hugechunks_seen_count__local;}
    #define INCREMENT_HUGECHUNK2_COUNT		++hugechunk_lookups_count__local
    #define INCREMENT_HUGECHUNK3_COUNT		++hugechunks_forwarded_count__local
#else
    #define COUNT_CODECHUNKS(sibid)		{}
    #define INCREMENT_HUGECHUNK2_COUNT		{}
    #define INCREMENT_HUGECHUNK3_COUNT		{}
#endif



#ifdef COUNT_COARSE_INTER_AGEGROUP_POINTERS_MAP_CARDS						// "COUNT_COARSE_INTER_AGEGROUP_POINTERS_MAP_CARDS" otherwise appears only in:    src/c/h/coarse-inter-agegroup-pointers-map.h
    //
    static unsigned long card_count1_local[ MAX_AGEGROUPS ];
    static unsigned long card_count2_local[ MAX_AGEGROUPS ];
    //
    #define COUNT_CARD1(i)	(card_count1_local[ i ]++)
    #define COUNT_CARD2(i)	(card_count2_local[ i ]++)
    //
#else
    #define COUNT_CARD(i)	{}
    #define COUNT_CARD1(i)	{}
    #define COUNT_CARD2(i)	{}
#endif



// Forward references:

 static int	   establish_all_necessary_empty_tospace_sib_buffers	(Task*          task,   int            youngest_agegroup_without_heapcleaning_request										);
//
#ifdef  BO_DEBUG
 static void       scan_memory_for_bogus_pointers			(Val_Sized_Unt* start,  Val_Sized_Unt* stop,                  int    age,                 int chunk_ilk								);
#endif
//
 static void       forward_all_root_chunks				(Task*          task,   Heap*          heap,                  Val**  roots,               int                                   max_cleaned_agegroup		);
 static void       forward_all_chunks_referenced_by_uncleaned_agegroups	(Task*          task,   Heap*          heap,                                              int                                   max_cleaned_agegroup		);
 static void       forward_all_remaining_live_chunks			(Heap*          heap,   int            max_cleaned_agegroup,  int    max_swept_agegroup										);

 static void       trim_heap		    				(Task*          task,   int            max_cleaned_agegroup													);
//
 static Val        forward_chunk					(Heap*          heap,   Sibid          max_sibid,             Val    chunk,               Sibid                                 id				);
 static Val        forward_special_chunk				(Heap*          heap,   Sibid          max_sibid,             Val*   chunk,               Sibid                                 id,                 Val tagword	);
//
 static Hugechunk* mark_hugechunk_as_live				(Heap*          heap,   int            max_agegroup,          Val    chunk,               Sibid                                 id				);

//


// Symbolic names for the sib buffers.
// This is used only for debug printouts
// for human consumption -- no code depends
// on these values:
//
 char*   sib_name__global   [ MAX_PLAIN_SIBS+1 ] =   { "new", "record", "pair", "string", "vector" };


/* DEBUG */
// static char *state_name[] = {"FREE", "YOUNG", "FORWARD", "OLD", "PROMOTE"};	// 2010-11-15 CrT: I commented this out because it is nowhere used.
//
static inline Punt  max   (Punt a,  Punt b)   {
    //               ===
    //
    if (a > b)   return a;
    else         return b;
}

//
static inline void  forward_pointee_if_in_fromspace   (Heap* heap,  Sibid* b2s,  Sibid max_sibid,  Val* pointer)   {
    //              ===============================
    //
    // If *pointer is in fromspace, forward (copy) it to to-space.

    Val pointee =  *pointer;

    if (IS_POINTER( pointee )) {								// Ignore Tagged_Int values.
	//
	Sibid  sibid =  SIBID_FOR_POINTER(b2s, pointee );					COUNT_CODECHUNKS( sibid );
	//
	if (SIBID_IS_IN_FROMSPACE( sibid, max_sibid )) {
	    //
	    *pointer =  forward_chunk( heap, max_sibid, pointee, sibid );
	}
    }
}
//
static void         forward_promote_or_reclaim_all_hugechunks                  (Heap* heap,  int oldest_agegroup_to_clean)   {
    //              =========================================
    //
    // Cycle over all hugechunks, forwarding, promoting or reclaiming
    // each one as appropriate.  Hugechunks are marked for promotion
    // or forwarding by mark_hugechunk_as_live().
    //
    // Dead records, pairs, strings and vectors are never explicitly
    // reclaimed;  they simply get left behind after we have copied all
    // live values from from-space to to-space, and get reclaimed en masse
    // when we release or re-use the from-space buffer.
    //
    // However, to save time and space we avoid copying Hugechunks
    // (i.e., Codechunks) because they are large and static; instead
    // of copying them we just change their hugechunk_state field.
    //
    // Consquently when hugechunks -do- become garbage we must in fact
    // explicitly free them.  That is our task here.
    //
    // Hugechunks which need to be forwarded or promoted have been
    // so marked by
    //
    //     mark_hugechunk_as_live ()
    //
    // At this point the remaining hugechunks not so marked are
    // garbage and can be reclaimed.
    //
    // We reclaim hugechunks agegroup by agegroup from oldest to
    // youngest so that we can promote them.

    Sibid*  b2s =  book_to_sibid__global;							// Cache global locally for speed.   book_to_sibid__global	def in    src/c/heapcleaner/heapcleaner-initialization.c

    for (int age = oldest_agegroup_to_clean;   age > 0;   --age) {
        //
	Agegroup*  promote_agegroup;
        //
	Agegroup*  ag = heap->agegroup[ age -1 ];
        //
	int forward_state;
	int promote_state;

	free_agegroup( heap, age-1 );								// free_agegroup				def in    src/c/heapcleaner/heapcleaner-stuff.c


        // NOTE: There should never be any hugechunk
        // with the SENIOR_HUGECHUNK_WAITING_TO_BE_PROMOTED tag in the oldest agegroup.
        //
	if (age == heap->active_agegroups) {

	    // Oldest agegroup chunks are promoted to the same agegroup:
            //
	    promote_agegroup = heap->agegroup[ age-1 ];

	    forward_state = JUNIOR_HUGECHUNK;							// Oldest active agegroup holds only YOUNG hugechunks.

	    promote_state = JUNIOR_HUGECHUNK;		// Is this value correct?  Added arbitrarily to resolve a gcc compiler warning. XXX QUERO FIXME

	} else {
	    //
	    promote_agegroup = heap->agegroup[ age ];

	    forward_state = SENIOR_HUGECHUNK;

	    if (age == oldest_agegroup_to_clean
            ||  age == heap->active_agegroups-1
	    )
		 promote_state =  JUNIOR_HUGECHUNK;
	    else promote_state =  SENIOR_HUGECHUNK;
		//
		// The chunks promoted from agegroup 'age' to agegroup 'age'+1, when
		// agegroup 'age'+1 is also being cleaned, are "OLD", thus we need
		// to mark the corresponding big chunks as old so that they do not
		// get out of sync.  Since the oldest agegroup has only YOUNG
		// chunks, we have to check for that case too.
	}

	for (int s = 0;   s < MAX_HUGE_SIBS;   ++s) {							// MAX_HUGE_SIBS (== 1)		def in    src/c/h/sibid.h
	    //
	    Hugechunk*  promote =  promote_agegroup->hugechunks[ s ];					// s = 0 == CODE__HUGE_SIB	def in    src/c/h/sibid.h
	    Hugechunk*  forward =  NULL;

	    Hugechunk*  next;

	    for (Hugechunk*
                p  = ag->hugechunks[ s ];
                p != NULL;
                p  = next
	    ){
		next = p->next;

		ASSERT( p->agegroup == age );

		switch (p->hugechunk_state) {
		    //
		case JUNIOR_HUGECHUNK:
		case SENIOR_HUGECHUNK:
		    free_hugechunk( heap, p );								// free_hugechunk		def in    src/c/heapcleaner/hugechunk.c
		    break;

		case JUNIOR_HUGECHUNK_WAITING_TO_BE_FORWARDED:
		    //
		    p->hugechunk_state = forward_state;
		    //
		    p->next  = forward;
		    forward   = p;
		    break;

		case SENIOR_HUGECHUNK_WAITING_TO_BE_PROMOTED:
		    //
		    p->hugechunk_state = promote_state;
		    //
		    p->next  = promote;
		    p->age++;
		    promote = p;
		    break;

		default:
		    die ("Strange hugechunk state %d @ %#x in agegroup %d\n", p->hugechunk_state, p, age);
                    exit(1);										// Cannot execute -- just to quiet gcc -Wall.
		}
	    }

	    promote_agegroup->hugechunks[ s ] = promote;			// A nop for the oldest agegroup.

	    ag->hugechunks[ s ] = forward;
	}
    }

    #ifdef BO_DEBUG
    // DEBUG
    for (int age = 0;  age < heap->active_agegroups;  age++) {
        //
	Agegroup*	ag =  heap->agegroup[age];
        //
	scan_memory_for_bogus_pointers( (Val_Sized_Unt*) ag->sib[ RO_POINTERS_SIB ]->tospace, (Val_Sized_Unt*) ag->sib[ RO_POINTERS_SIB ]->tospace.used_end, age+1, RO_POINTERS_SIB);
	scan_memory_for_bogus_pointers( (Val_Sized_Unt*) ag->sib[ RO_CONSCELL_SIB ]->tospace, (Val_Sized_Unt*) ag->sib[ RO_CONSCELL_SIB ]->tospace.used_end, age+1, RO_CONSCELL_SIB);
	scan_memory_for_bogus_pointers( (Val_Sized_Unt*) ag->sib[ RW_POINTERS_SIB ]->tospace, (Val_Sized_Unt*) ag->sib[ RW_POINTERS_SIB ]->tospace.used_end, age+1,  RW_POINTERS_SIB);
    }
    // DEBUG
    #endif


    // Re-label book_to_sibid__global entries for hugechunk regions to reflect promotions:
    //
    for (Hugechunk_Quire* hq = heap->hugechunk_quires;  hq != NULL;  hq = hq->next) {
	//
	// If the minimum age of the live chunks in
	// the region is less than or equal to oldest_agegroup_to_clean
	// then it is possible that it has increased
	// as a result of promotions or freeing of chunks.

	if (hq->age_of_youngest_live_chunk_in_quire <= oldest_agegroup_to_clean) {
	    //
	    int min = MAX_AGEGROUPS;

	    for (int i = 0;  i < hq->page_count;  ) {
		//
		Hugechunk* p  = hq->hugechunk_page_to_hugechunk[ i ];

		if (!HUGECHUNK_IS_FREE( p )									// HUGECHUNK_IS_FREE				def in    src/c/h/heap.h
		&&  p->age < min
		){
		    min = p->age;
		}

		i += hugechunk_size_in_hugechunk_ram_quanta( p );							// hugechunk_size_in_hugechunk_ram_quanta	def in   src/c/h/heap.h
	    }

	    if (hq->age_of_youngest_live_chunk_in_quire != min) {
		hq->age_of_youngest_live_chunk_in_quire  = min;

		set_book2sibid_entries_for_range (
		    //
		    b2s,
		    (Val*) hq,
		    BYTESIZE_OF_QUIRE( hq->quire ),
		    HUGECHUNK_DATA_SIBID( min )
		);

		b2s[ GET_BOOK_CONTAINING_POINTEE( hq ) ]
		    =
		    HUGECHUNK_RECORD_SIBID( min );
	    }
	}
    }

}

//
static void         update_fromspace_seniorchunks_end_pointers   (Heap* heap, int oldest_agegroup_to_clean) {
    //              ======================================
    //
    // Background:  We require that a chunk survive two heapcleans in
    // a given agegroup before being promoted to the next agegroup.
    //
    // To this end, we divide the chunks in a given agegroup sib into
    // "young" (have not yet survived a heapclean) and
    // "old" (have survived one heapclean).  This pointer tracks the
    // boundary between old and new;  Chunks before this get promoted
    // if they survive the next heapcleaning; those beyond it do not.
    //
    // Special case: chunks in the oldest active agegroup are forever young.
    //
    // We are called at the end of a heapcleaning;  Our job is to
    // appropriately update the 'fromspace.seniorchunks_end' pointers
    // marking the boundary between 'old' and 'young' chunks in
    // each sib in each agegroup.
    //
    for (int a = 0;  a < oldest_agegroup_to_clean;  a++) {
        //
	Agegroup*  age =   heap->agegroup[ a ];
        //
	if (a == heap->active_agegroups - 1) {
	    //
            // The oldest agegroup has only "young" chunks:
            //
	    for (int s = 0;   s < MAX_PLAIN_SIBS;   ++s) {							// sib_is_active	def in    src/c/h/heap.h
	        //
		if (sib_is_active( age->sib[ s ]))  age->sib[ s ]->fromspace.seniorchunks_end =  age->sib[ s ]->tospace.start;
		else		                    age->sib[ s ]->fromspace.seniorchunks_end =  NULL;
	    }

	} else {

	    for (int s = 0;   s < MAX_PLAIN_SIBS;   ++s) {
	        //
		if (sib_is_active( age->sib[ s ] ))  age->sib[ s ]->fromspace.seniorchunks_end =  age->sib[ s ]->tospace.used_end;
		else		                     age->sib[ s ]->fromspace.seniorchunks_end =  NULL;
	    }
	}
    }
}

//
static void         do_end_of_heapcleaning_statistics_stuff   (Task* task,  Heap* heap,  int max_swept_agegroup,  int oldest_agegroup_to_clean,  Val** tospace_limit)   {
    //              =======================================
    //
    // Cleaner statistics:

    // Count the number of forwarded bytes:
    //
    if (max_swept_agegroup != oldest_agegroup_to_clean) {
	//
	Agegroup*	ag =  heap->agegroup[ max_swept_agegroup -1 ];
	//
	for (int s = 0;  s < MAX_PLAIN_SIBS;  s++) {
	    //
	    INCREASE_BIGCOUNTER(
		//
		&heap->total_bytes_copied_to_sib[ max_swept_agegroup-1 ][ s ],
		ag->sib[ s ]->tospace.used_end - tospace_limit[ s ]
	    );
	}
    }
    for (    int a = 0;  a < oldest_agegroup_to_clean;  a++) {
	for (int s = 0;  s < MAX_PLAIN_SIBS;  s++) {
	    //
	    Sib* ap =  heap->agegroup[ a ]->sib[ s ];
	    //
	    if (sib_is_active(ap)) {							// sib_is_active	def in    src/c/h/heap.h
		//
		INCREASE_BIGCOUNTER(
		    //
		    &heap->total_bytes_copied_to_sib[ a ][ s ],
		    ap->tospace.used_end - tospace_limit[ s ]
		);
	    }
	}
    }


    #if NEED_HUGECHUNK_REFERENCE_STATISTICS
        //
        debug_say ("hugechunk stats: %d seen, %d lookups, %d forwarded\n",    hugechunks_seen_count__local, hugechunk_lookups_count__local, hugechunks_forwarded_count__local);
    #endif

    #if NEED_HEAPCLEANER_PAUSE_STATISTICS	// Don't do timing when collecting pause data.
	if (heapcleaner_messages_are_enabled__global) {
	    long	                         cleaning_time;
	    stop_heapcleaning_timer (task->pthread, &cleaning_time);				// stop_heapcleaning_timer	is from   src/c/main/timers.c
	    debug_say (" (%d ms)\n",             cleaning_time);
	} else {
	    stop_heapcleaning_timer (task->pthread, NULL);
	}
    #endif
}

//
static int          prepare_for_heapcleaning    (int* max_swept_agegroup,  Val** tospace_limit, Task* task,  Heap* heap,  int level)   {
    //              ========================
    //
    //
    #if !NEED_HEAPCLEANER_PAUSE_STATISTICS								// Don't do timing when collecting pause data.
	//
	start_heapcleaning_timer( task->pthread );							// start_heapcleaning_timer	def in    src/c/main/timers.c
    #endif

    #if NEED_HUGECHUNK_REFERENCE_STATISTICS
	//
        hugechunks_seen_count__local      =
        hugechunk_lookups_count__local    =
        hugechunks_forwarded_count__local = 0;
    #endif

    // Decide how many agegroups to clean
    // (always at least as many as requested)
    // and ensure that all necessary to-space
    // sib buffers have enough free space to
    // handle worst-case space requirements:
    //
    int oldest_agegroup_to_clean
	=
	establish_all_necessary_empty_tospace_sib_buffers( task, level );


    // We sweep one more agegroup than we clean,
    // except when we're cleaning all agegroups:
    //
    if (oldest_agegroup_to_clean >= heap->active_agegroups) {
	//
	*max_swept_agegroup = oldest_agegroup_to_clean;

    } else {

	*max_swept_agegroup = oldest_agegroup_to_clean+1;

	// Heapcleaner statistics:
	//
	// Remember the top of to-space for max_swept_agegroup:
	//
	for (int ilk = 0;  ilk < MAX_PLAIN_SIBS;  ilk++) {
	    //
	    tospace_limit[ ilk ]
		=
		heap->agegroup[ *max_swept_agegroup-1 ]->sib[ ilk ]->tospace.used_end;
	}
    }

    // A little statistics gathering:
    //
    note_active_agegroups_count_for_this_timesample( oldest_agegroup_to_clean );				// note_active_agegroups_count_for_this_timesample		def in    src/c/heapcleaner/heapcleaner-statistics.h
    //
    #if !NEED_HEAPCLEANER_PAUSE_STATISTICS	// Don't do messages when collecting pause data.
	//
	if (heapcleaner_messages_are_enabled__global) {
	    //	
	    debug_say ("GC #");
	    //	
	    for (int age = heap->active_agegroups-1;  age >= 0;   age--) {
		//
		debug_say ("%d.", heap->agegroup[age]->heapcleanings_count);
	    }
	    debug_say ("%d:  ", heap->agegroup0_heapcleanings_count);
	}
    #endif

    return oldest_agegroup_to_clean;
}

//
void                heapclean_n_agegroups   (Task* task,  Val** roots,  int level)   {
    //              =====================
    // 
    // Clean (at least) the first 'level' agegroups.
    //
    // By definition, level should be at least 1.
    //
    // This function is called (only) from
    //     src/c/heapcleaner/call-heapcleaner.c

    Heap*  heap  =  task->heap;

    Val*  tospace_limit[ MAX_PLAIN_SIBS ];	// Set by following call.			// Heapcleaner statistics:  Counts number of bytes forwarded.
    int	  max_swept_agegroup;			// Set by following call.

    int oldest_agegroup_to_clean
        =
        prepare_for_heapcleaning(
	    //
            &max_swept_agegroup,		// Return value.
	    tospace_limit,			// Return value.
	    //
	    task,
	    heap,
            level
        );

    // Start the cleaning by forwarding (copying from from-space
    // to to-space) all heap values (in the agegroups we are cleaning)
    // which are directly referenced by Mythryl-task registers, or
    // by global registers in the C runtime:
    //
    forward_all_root_chunks( task, heap, roots, oldest_agegroup_to_clean );

    // Now forward all heap values in the agegroups we are cleaning
    // which are directly referenced from the agegroups we are NOT
    // cleaning:
    //
    forward_all_chunks_referenced_by_uncleaned_agegroups( task, heap, oldest_agegroup_to_clean );

    forward_all_remaining_live_chunks( heap, oldest_agegroup_to_clean, max_swept_agegroup );

    null_out_newly_dead_weakrefs( heap );									// null_out_newly_dead_weakrefs					def in    src/c/heapcleaner/heapcleaner-stuff.c

    forward_promote_or_reclaim_all_hugechunks( heap, oldest_agegroup_to_clean );

    update_fromspace_seniorchunks_end_pointers( heap, oldest_agegroup_to_clean);

    do_end_of_heapcleaning_statistics_stuff( task,  heap,  max_swept_agegroup,  oldest_agegroup_to_clean,  tospace_limit);

    #ifdef CHECK_HEAP
	check_heap( heap, max_swept_agegroup );
    #endif

    trim_heap( task, oldest_agegroup_to_clean );
}								// fun heapclean_n_agegroups.


// DEBUG
#ifdef  BO_DEBUG
//
static void         scan_memory_for_bogus_pointers                        (Val_Sized_Unt* start,  Val_Sized_Unt* stop,  int age,  int chunk_ilk)   {
    //              ==============================
    //
    // A debug support fn.
    //
    //
    Sibid*  b2s =  book_to_sibid__global;							// Cache global locally for speed.   book_to_sibid__global	def in    src/c/heapcleaner/heapcleaner-initialization.c

    for ( ;   start < stop;  ++start) {
	//
	Val_Sized_Unt w = *start;
	//
	if (IS_POINTER( w )) {
	    //
	    int   index = GET_BOOK_CONTAINING_POINTEE( w );

	    Sibid sibid    = b2s[ index ];

	    switch (GET_KIND_FROM_SIBID(sibid)) {
	        //
	    case CODE_KIND:
		//
		while (!SIBID_ID_IS_BIGCHUNK_RECORD(sibid)) {
		    //
		    sibid = b2s[ --index ];
		}

		Hugechunk_Quire*
		    //
		    hq =  (Hugechunk_Quire*)  ADDRESS_OF_BOOK( index );					// ADDRESS_OF_BOOK	def in    src/c/h/sibid.h

		Hugechunk*
		    //
		    p =  get_hugechunk_holding_pointee( hq, w );

		if (p->state == FREE_HUGECHUNK) {
		    //
		    debug_say ("** [%d/%d]: %#x --> %#x; unexpected free hugechunk\n", age, chunk_ilk, start, w);
		}
		break;

	    case RO_POINTERS_KIND:
	    case RO_CONSCELL_KIND:
	    case NONPTR_DATA_KIND:
	    case RW_POINTERS_KIND:
		break;

	    default:
		if (sibid != UNMAPPED_BOOK_SIBID) {
		    debug_say ("** [%d/%d]: %#x --> %#x; strange chunk ilk %d\n", age, chunk_ilk, start, w, GET_KIND_FROM_SIBID(sibid));
		}
		break;
	    }
	}
    }
}
#endif // BO_DEBUG
//
//
//
static int          establish_all_necessary_empty_tospace_sib_buffers       (Task* task,   int youngest_agegroup_without_heapcleaning_request)   {
    //              =================================================
    // 
    // Make sure that every to-space sib buffer in every age group
    // has sufficient space to guarantee that the heapcleaning
    // we are about to do cannot possibly overflow any of them.
    //
    // We return the number of agegroups to heapclean.
    //
    // We are called in exactly one place -- near the top of
    //     prepare_for_heapcleaning()
    // in this file which in turn is called (only) at the top of
    //     heapclean_n_agegroups
    // in this file.
    //
    // The to-space buffer for each sib must be large enough
    // to hold all possible incoming data from three different
    // sources:
    //
    //  1) Live "young" chunks in the fromspace of the sib will
    //     be copied into the tospace buffer.  (We don't have
    //     to worry about "old" chunks in our fromspace;  if
    //     they are live they will be promoted to another agegroup.)
    //
    //  2) Live "old" chunks from the corresponding sib in the
    //     next-youngest agegroup -- these will be promoted into
    //     our tospace buffer.
    //
    //  3) User code may be in the process of creating one or more
    //     new chunks in the sib;  if so, the tospace buffer must
    //     reserve enough additional space to contain them.
    //
    // We will at minimum clean all agegroups which client
    // code has requested us to clean;  we may also clean
    // additional agegroups in order to rebalance the size
    // distribution of the agegroup buffers per policy.
    //
    // In some cases we may be able to re-use a saved buffer
    // from a previous heapcleaning; otherwise we must allocate
    // new ram to hold the to-space buffers.   We're usually
    // allocating megabytes of space, so we do not use malloc(),
    // but rather obtain_quire_from_os() -- see
    //     set_up_tospace_sib_buffers_for_agegroup()
    // in
    //     src/c/heapcleaner/heapcleaner-stuff.c
    //
    // We are not called unless at least agegroup 1 is being heapcleaned,
    // so we know that youngest_agegroup_without_heapcleaning_request > 1.

    Heap* heap = task->heap;

    int younger_agegroup_heapcleanings_since_last_heapcleaning;		// How many heapcleanings has the next-youngest agegroup had
									// since we last heapcleaned the current agegroup?
									// We want each agegroup to do about 1/10 as many heapcleanings
									// as the next-youngest one;  We'll us this variable to compute
									// how close we came this time around for this agegroup.

    Punt  bytes_of_oldstuff_in_next_younger_agegroup[ MAX_PLAIN_SIBS ];	// This tracks, for each agegroup.sib, the total bytes of
									// "old" stuff in the corresponding next-younger sib.
									// The significance of this is that during the about-to-happen
									// heapcleaning potentially all of this "old" stuff might get
									// promoted (copied) into our sib, so we *must* arrange to
									// have enough free space to accept that many bytes from it.

    Punt  min_bytesize_for_sib[       MAX_PLAIN_SIBS ];			// Minimum bytesize for each sib buffer:  If we cannot get this much we will groan and die.
									// Usually we will allocate more than this;  this is our emergency fallback position if we
									// Cannot get as much ram from the host OS as we really want.

    // Initialize bytes_of_oldstuff_in_next_younger_agegroup[].
    // Since agegroup0 is a special case -- it does not have
    // separate sib buffers -- we conservatively assume that,
    // during the about-to-happen heapcleaning, each agegroup1
    // sib buffer may have to accept the entire contents of
    // the agegroup0 buffer (which we assume to be full -- it
    // will normally be within a few KB of totally full before
    // heapcleaning is initiated):
    //
    for (int s = 0;  s < MAX_PLAIN_SIBS;  ++s) {
        //
	bytes_of_oldstuff_in_next_younger_agegroup[ s ]
	    =
            heap->agegroup0_master_buffer_bytesize;			// This value doesn't seem right, but it works and my attention is elsewhere, so for the moment I'm going to accept this as Black Magic.  -- 2012-01-31 CrT
//          heap->agegroup0_master_buffer_bytesize / MAX_PTHREADS;	// This seems more appropriate, but plugging it in hangs the compiler.  -- 2012-01-31 CrT
//	    task->heap_allocation_buffer_bytesize;			// This seems more appropriate, but plugging it in hangs the compiler.  -- 2012-01-31 CrT
//	    agegroup0_buffer_size_in_bytes( task );			// This seems more appropriate, but plugging it in hangs the compiler.  -- 2012-01-31 CrT

    }

    int younger_agegroup_heapcleanings_count				// In general this holds  ag->heapcleanings_count
	=								// for the agegroup one younger than the one we're
	heap->agegroup0_heapcleanings_count;				// currently processing in the loop, but agegroup0
									// is a special case. 

    // Over all agegroups we are required to heapclean,
    // plus any agegroups we choose to heapclean in addition:
    //
    for (int age = 0;  age < heap->active_agegroups;  ++age) {
        //
	Agegroup* ag =  heap->agegroup[ age ];

        // Check to see if agegroup (age+1)
        // should be flipped:
															// sib_is_active		def in    src/c/h/heap.h
															// sib_freespace_in_bytes	def in    src/c/h/heap.h
	// Decide whether to heapclean this agegroup.
	// We heapclean it in either of two cases:
	//
	//  o Caller ordered us to do so.
	//
	//  o Heapcleaning the next-younger generation
	//    might promote more "old" stuff into our
	//    buffer than we current have room for.
	//
	if (age >= youngest_agegroup_without_heapcleaning_request) {
	    //
	    // We are not required to heapclean this agegroup,
	    // so we can skip heapcleaning it, provided that
	    // all of our sib buffers have enough free space
	    // to accept the maximum amount of oldstuff which
	    // could possibly be promoted from the next-younger
	    // agegroup during its own heapcleaning:
	    //
	    int want_to_heapclean_this_agegroup = FALSE;								// Initial assumption.
	    //
	    for (int s = 0;  s < MAX_PLAIN_SIBS;  ++s) {
	        //
		Sib* sib =  ag->sib[ s ];

		Punt free_bytes_in_sib											// Compute free bytes in this sib buffer.
		    =
		    sib_is_active( sib )
                      ? sib_freespace_in_bytes( sib )
		      : 0;

		if (free_bytes_in_sib < bytes_of_oldstuff_in_next_younger_agegroup[ s ]) {				// Is this to ensure that we have enough room to promote the entire younger generation into this buffer?
		    //
		    want_to_heapclean_this_agegroup = TRUE;								// Insufficient free space in sib buffer, must create more.
		    break;
		}
	    }

	    if (!want_to_heapclean_this_agegroup)   return age;
	}

	/////////////////////////////////////
	// We need heapclean agegroup[ age ].
	/////////////////////////////////////

	younger_agegroup_heapcleanings_since_last_heapcleaning
	    =
	    younger_agegroup_heapcleanings_count
	    -
	    ag->heapcleanings_count_of_younger_agegroup_during_last_heapcleaning;

	// Compute the space requirements for this agegroup,
	// make the old to-space into from-space, and
	// allocate a new to-space.
	//
	for (int s = 0;  s < MAX_PLAIN_SIBS;  ++s) {
	    //	
	    Punt  bytes_of_youngstuff_in_sib;

	    Sib* sib =  ag->sib[ s ];

	    if (sib_is_active( sib )) {									// sib_is_active			def in    src/c/h/heap.h
		//
		make_sib_tospace_into_fromspace( sib );							// Sets fromspace.start, fromspace.bytesize and fromspace.used_end.
													// make_sib_tospace_into_fromspace	def in    src/c/h/heap.h
		bytes_of_youngstuff_in_sib
		    =
		    (Punt)  sib->fromspace.used_end
                    -
                    (Punt)  sib->fromspace.seniorchunks_end;

	    } else {

		sib->fromspace.bytesize = 0;  								// To ensure accurate stats.

		if (sib->requested_extra_free_bytes == 0
		    &&
		    bytes_of_oldstuff_in_next_younger_agegroup[ s ] == 0
		){
		    continue;										// We don't actually need *any* freespace in this sib buffer(!) so quit worrying about it.
		}

		bytes_of_youngstuff_in_sib = 0;
	    }

	    Punt min_bytes_for_sib									// Absolute minimum bytesize for sib's tospace buffer; We can't keep running without this much.
		=
		sib->requested_extra_free_bytes								// Caller wants this many free bytes in this sib for some new chunk it is creating.
		+
		bytes_of_oldstuff_in_next_younger_agegroup[ s ]						// This many bytes of oldstuff might get promoted from next-younger agegroup's sib into this one.
		+
		bytes_of_youngstuff_in_sib;								// Our youngstuff will be copied into the tospace buffer. (Our oldstuff will be promoted to next agegroup -- not our problem.)

	    if (s == RO_CONSCELL_SIB)   min_bytes_for_sib += 2*WORD_BYTESIZE;				// First slot isn't used, but may need the space for poly-equal.

	    min_bytesize_for_sib[ s ] =  min_bytes_for_sib;

	    // 'min_bytes_for_sib' is the least we can live with, but in general we would like
	    // to have more space than that.  Here we compute just how much more.
	    //	
	    // The desired size is one that will allow "target_heapcleaning_frequency_ratio"
	    // heapcleanings of the previous agegroup before this has to be cleaned again.
	    // We approximate this as (f/n)*r, where
	    //   r == target_heapcleaning_frequency_ratio
	    //   f == # of bytes of youngstuff in this sib.
	    //   n == # of cleanings of the previous agegroup since the last heapcleaning of this agegroup.
	    // We also need to allow space for young chunks in this agegroup,
	    // but the new size shouldn't exceed the maximum size for the
	    // sib buffer (unless min_bytes_for_sib > soft_max_bytesize).
	    //
	    Punt  preferred_bytesize_for_sib
	        =
		sib->requested_extra_free_bytes								// Caller wants this many free bytes in this sib for some new chunk it is creating.
		+
                bytes_of_oldstuff_in_next_younger_agegroup[ s ]						// This many bytes of oldstuff might get promoted from next-younger agegroup's sib into this one.
		+
		(bytes_of_youngstuff_in_sib / younger_agegroup_heapcleanings_since_last_heapcleaning) * ag->target_heapcleaning_frequency_ratio;

	    // Clamp preferred_bytesize_for_sib to sane range:
	    //
	    if (preferred_bytesize_for_sib < min_bytes_for_sib) {
		preferred_bytesize_for_sib = min_bytes_for_sib;
	    }
	    if (preferred_bytesize_for_sib >      sib->soft_max_bytesize) {
		preferred_bytesize_for_sib = max( sib->soft_max_bytesize, min_bytes_for_sib);
	    }

	    if (preferred_bytesize_for_sib == 0) {
		//
		sib->tospace.used_end	=  NULL;
		sib->tospace.limit	=  NULL;
		sib->tospace.bytesize	=  0;

	    } else {

		sib->tospace.bytesize
		    =
		    BOOKROUNDED_BYTESIZE( preferred_bytesize_for_sib );							// Round up to a multiple of 64KB.
	    }

	    // Note: any data between sib->fromspace.seniorchunks_end
	    // and sib->tospace.used_end is "young",
	    // and should stay in this agegroup.
	    //
	    if (sib->fromspace.bytesize > 0)   bytes_of_oldstuff_in_next_younger_agegroup[ s ] =   (Punt) sib->fromspace.seniorchunks_end - (Punt) sib->fromspace.start;
	    else 		               bytes_of_oldstuff_in_next_younger_agegroup[ s ] =   0;
	}

	ag->heapcleanings_count_of_younger_agegroup_during_last_heapcleaning
	    =
            younger_agegroup_heapcleanings_count;

	++ ag->heapcleanings_count;

	younger_agegroup_heapcleanings_count
	    =
	    ag->heapcleanings_count;

	ag->fromspace_quire =  ag->tospace_quire;

	if (!set_up_tospace_sib_buffers_for_agegroup( ag )) {							// set_up_tospace_sib_buffers_for_agegroup				def in   src/c/heapcleaner/heapcleaner-stuff.c
	    //
	    // We were unable to allocate sufficient RAM from the OS. (?!)
            // Try to allocate the minimum size:

	    say_error( "Unable to allocate to-space for agegroup %d; trying smaller size\n", age+1 );

	    for (int s = 0;   s < MAX_PLAIN_SIBS;   ++s) {
		//
		ag->sib[ s ]->tospace.bytesize
		    =
		    BOOKROUNDED_BYTESIZE( min_bytesize_for_sib[ s ] );
	    }

	    if (!set_up_tospace_sib_buffers_for_agegroup( ag )) {
		//
		die("Unable to allocate minimum size sib buffers for agegroup\n");				// Let's be more specific here! XXX BUGGO FIXME.
	    }
	}


        if (sib_is_active( ag->sib[ RW_POINTERS_SIB ])) {							// sib_is_active						def in    src/c/h/heap.h
	    //
	    make_new_coarse_inter_agegroup_pointers_map_for_agegroup( ag );					// make_new_coarse_inter_agegroup_pointers_map_for_agegroup	def in    src/c/heapcleaner/heapcleaner-stuff.c
	}
    }														// for (int age = 0;   age < heap->active_agegroups;   ++age) 

    return heap->active_agegroups;
}														// fun establish_all_necessary_empty_tospace_sib_buffers

//
//
static void         forward_all_root_chunks (
    //              =======================
    //
    Task*  task,
    Heap*  heap,
    Val**  roots,
    int	   oldest_agegroup_to_clean
){
    // Our job here is to forward all "roots" -- all heap
    // chunks (of appropriate age) which are directly
    // accessible to user code.
    //
    // Our roots consist of saved Mythryl-task registers
    // and C global variables pointing into the Mythryl heap.
    // They are enumerated for us by code in    clean_heap().							// clean_heap							def in    src/c/heapcleaner/call-heapcleaner.c 
    //

    Sibid* b2s =  book_to_sibid__global;									// Cache global locally for speed.   book_to_sibid__global	def in    src/c/heapcleaner/heapcleaner-initialization.c

    Sibid  max_sibid =  MAKE_MAX_SIBID( oldest_agegroup_to_clean );						// MAKE_MAX_SIBID						is from   src/c/h/sibid.h

    Val*    root;
    while ((root = *roots++) != NULL) {
	//
	forward_pointee_if_in_fromspace( heap, b2s, max_sibid, root );
    }
}														// fun forward_all_root_chunks

    
static void         forward_all_chunks_referenced_by_uncleaned_agegroups   (
    //              ====================================================
    Task*  task,
    Heap*  heap,
    int	   oldest_agegroup_to_clean
){
    // Our job here is to forward (copy from from-space
    // to to-space) all heap values in the agegroups we
    // are cleaning which are directly referenced from
    // the agegroups we are NOT cleaning.
    //
    // Any such references must be in the RW_POINTERS_SIB buffer:
    //
    //   o We clean youngest-first, so any such references
    //     must be from an older to younger agegroup.
    //
    //  o This can only happen if the older chunk was modified
    //    after creation.
    //
    //  o Only refcells and rw_vectors may be modified after creation.
    //
    //  o Except in agegroup0, all refcells and rw_vectors live in
    //    RW_POINTERS_SIB sib buffers.  (At this level a refcell is regarded
    //    as just a length-1 rw_vector.) 
    //
    // Consequently we need only check RW_POINTERS_SIB sib buffers in
    // this function.  (This is the main reason for having a separate
    // RW_POINTERS_SIB.)

    // Cache global in register for speed:
    //
    Sibid* b2s =  book_to_sibid__global;									// Cache global locally for speed.   book_to_sibid__global	def in    src/c/heapcleaner/heapcleaner-initialization.c

    Sibid  max_sibid =  MAKE_MAX_SIBID( oldest_agegroup_to_clean );

    // Scan the coarse_inter_agegroup_pointers_map cards
    // for all the agegroups we are NOT cleaning.
    //
    for (int age = oldest_agegroup_to_clean;   age < heap->active_agegroups;   ++age) {				// Remember that agegroup i lives in heap->agegroup[ i-1 ].
        //
	Agegroup* ag =  heap->agegroup[ age ];

	#ifdef COUNT_COARSE_INTER_AGEGROUP_POINTERS_MAP_CARDS
	    /*CARD*/ card_count1_local[age]=card_count2_local[age]=0;
	#endif

	if (!sib_is_active( ag->sib[ RW_POINTERS_SIB ]))   continue;						// sib_is_active			def in    src/c/h/heap.h

	Coarse_Inter_Agegroup_Pointers_Map*
	    //
	    map =  ag->coarse_inter_agegroup_pointers_map;

	if (map == NULL)   continue;

	Val* max_sweep =  ag->sib[ RW_POINTERS_SIB ]->tospace.swept_end;


	// Find all cards in this agegroup which contain
        // pointers into an agegroup no older than oldest_agegroup_to_clean:
	//
	int dirty_card;
	FOR_ALL_DIRTY_CARDS_IN_CARDMAP( map, oldest_agegroup_to_clean, dirty_card, {				// FOR_ALL_DIRTY_CARDS_IN_CARDMAP	def in    src/c/h/coarse-inter-agegroup-pointers-map.h
	    //
	    Val*  p =  (map->base_address + (dirty_card * CARD_SIZE_IN_WORDS));					// Get pointer to start of dirty card.
	    Val*  q =  p + CARD_SIZE_IN_WORDS;									// Get pointer to end   of dirty card.

	    int min_age = age+1;

	    COUNT_CARD1( age );											// Statistics.   COUNT_CARD1 def at top of file.

	    // Don't sweep above the allocation high-water mark:
	    //
	    if (q > max_sweep) {
		q = max_sweep;
	    }

	    // Over all (valid) words in the card:
	    //
	    for (;  p < q;  p++) {
		//
		Val word = *p;

		if (!IS_POINTER( word ))   continue;								// We're only interested in pointers -- specifically pointers into agegroups we're heapcleaning.

		Sibid  sibid =  SIBID_FOR_POINTER(b2s, word );							// Start figuring out what 'word' points to.

		COUNT_CODECHUNKS( sibid );									// Statistics.   COUNT_CODECHUNKS def at top of file.

		if (SIBID_IS_IN_FROMSPACE(sibid, max_sibid)) {							// If this is a from-space chunk.  SIBID_IS_IN_FROMSPACE  def in   src/c/h/sibid.h
		    //
		    COUNT_CARD2( age );										// Statistics.   COUNT_CARD1 def at top of file.

		    int target_age;

		    if (SIBID_KIND_IS_CODE( sibid )) {
			//
			Hugechunk* p =  mark_hugechunk_as_live( heap, oldest_agegroup_to_clean, word, sibid );	// Actual forwarding/promotion (as appropriate) is done in a postpass -- see forward_promote_or_reclaim_all_hugechunks()

			target_age = p->age;

		    } else {
			//
			*p = word = forward_chunk( heap, max_sibid, word, sibid );				// The beef in our burger.

			target_age = GET_AGE_FROM_SIBID( SIBID_FOR_POINTER(b2s, word ) );
		    }

		    if (min_age > target_age) {
			min_age = target_age;
		    }
		}
	    }
														// CLEAN_CARD	def in    src/c/h/coarse-inter-agegroup-pointers-map.h
	    // Re-mark the card:
	    //
	    ASSERT( map->min_age[ dirty_card ] <= min_age );
	    //
	    if      (age >= min_age)		            map->min_age[ dirty_card ] =  min_age;
	    else if (age == oldest_agegroup_to_clean)	    map->min_age[ dirty_card ] =  CLEAN_CARD;
	});
    }														// for (int age = oldest_agegroup_to_clean;  age < heap->active_agegroups;  age++) 

    #ifdef COUNT_COARSE_INTER_AGEGROUP_POINTERS_MAP_CARDS
	/*CARD*/debug_say ("\n[%d] SWEEP: ", oldest_agegroup_to_clean);
	/*CARD*/for (int age = oldest_agegroup_to_clean;  age < heap->active_agegroups;  age++) {
	/*CARD*/   Coarse_Inter_Agegroup_Pointers_Map* map = heap->agegroup[ age ]->coarse_inter_agegroup_pointers_map;
	/*CARD*/   if (age > oldest_agegroup_to_clean) debug_say (", ");
	/*CARD*/   debug_say ("[%d] %d/%d/%d", age+1, card_count1_local[age], card_count2_local[age], (map != NULL) ? map->card_count : 0);
	/*CARD*/}
	/*CARD*/debug_say ("\n");
    #endif

}														// fun forward_all_chunks_referenced_by_uncleaned_agegroups

//
//
static Bool         forward_rest_of_ro_pointers_sib   (								// Called only from forward_all_remaining_live_chunks (below).
    //              ===============================
    //
    Agegroup* ag,
    Heap*     heap,
    int       s,		// Either RO_POINTERS_SIB or RO_CONSCELL_SIB.
    Sibid     max_sibid
){
    // Forward (copy) to to-space all live values referenced
    // by the unscanned part of our tospace buffer.
    //
    // Return TRUE iff we did anything.
    //
    Sibid* b2s =  book_to_sibid__global;										// Cache global for speed.   book_to_sibid__global	def in    src/c/heapcleaner/heapcleaner-initialization.c
    //
    Bool made_progress = FALSE;

    Sib* sib = (ag)->sib[ s ];

    if (!sib_is_active( sib ))                      return FALSE;							// sib_is_active	def in    src/c/h/heap.h

    Val* p =  sib->tospace.swept_end;
    if  (p == sib->tospace.used_end)   return FALSE;

    Val* q;

    made_progress = TRUE;												// Do we really need this?  Won't it work to return TRUE only if we actually forwarded something? XXX BUGGO FIXME
    do {
	q = sib->tospace.used_end;

	for (;  p < q;  p++)   forward_pointee_if_in_fromspace(heap,b2s,max_sibid, p );

    } while (q != sib->tospace.used_end);

    sib->tospace.swept_end = q;										// Remember where to pick up next time we're called.

    return   made_progress;
}
//
static Bool         forward_rest_of_rw_pointers_sib              (Agegroup* ag,  Heap* heap,  int oldest_agegroup_to_clean)   {	// Called only from forward_all_remaining_live_chunks (below).
    //              ===============================
    // 
    // Forward (copy) to to-space all live values referenced
    // by the unscanned part of our RW_POINTERS_SIB.
    //
    // Return TRUE iff we did anything.


    Sib* sib =   ag->sib[ RW_POINTERS_SIB ];

    if (!sib_is_active( sib ))   return FALSE;										// sib_is_active	def in    src/c/h/heap.h

    Coarse_Inter_Agegroup_Pointers_Map* map
	= 
        ag->coarse_inter_agegroup_pointers_map;

    Sibid*	   b2s    =  book_to_sibid__global;									// Cache locally for speed.   book_to_sibid__global	def in    src/c/heapcleaner/heapcleaner-initialization.c

    Sibid	   max_sibid   =  MAKE_MAX_SIBID(oldest_agegroup_to_clean);

    Val_Sized_Unt  cardmask =  ~(CARD_BYTESIZE - 1);

    Val	 w;

    Val* card_start;
    int  card_mark;

    // Sweep a single card at a time,
    // looking for references that need
    // to be remembered.

    int this_age = GET_AGE_FROM_SIBID( sib->id );

    Val* p =  sib->tospace.swept_end;

    if (p == sib->tospace.used_end)   return FALSE;

    while (p < sib->tospace.used_end) {
        //
	Val* stop = (Val*) (((Punt)p + CARD_BYTESIZE) & cardmask);

	if (stop > sib->tospace.used_end) {
	    stop = sib->tospace.used_end;
        }
        // Sweep the next page until we see
        // a reference to a younger agegroup:
        //
	card_start = p;
	card_mark = CARDMAP_MIN_AGE_VALUE_FOR_POINTER(map, card_start);

	while (p < stop) {
	    //
	    if (IS_POINTER( w = *p)) {
	        //
		Sibid sibid = SIBID_FOR_POINTER( b2s, w );

		int target_age;

		COUNT_CODECHUNKS( sibid );

		if (SIBID_IS_IN_FROMSPACE( sibid, max_sibid )) {
		    //
                    // This is a from-space chunk.

		    if (SIBID_KIND_IS_CODE( sibid )) {
		        //
			Hugechunk*  p =   mark_hugechunk_as_live( heap, oldest_agegroup_to_clean, w, sibid );	// Actual forwarding/promotion (as appropriate) is done in a postpass -- see forward_promote_or_reclaim_all_hugechunks()
			target_age = p->age;

		    } else {

			*p = w = forward_chunk(heap, max_sibid, w, sibid);

			target_age = GET_AGE_FROM_SIBID( SIBID_FOR_POINTER( b2s, w ));
		    }

		    if (card_mark > target_age) {
			card_mark = target_age;
		    }
		}
	    }
	    p++;
	}								// while (p < stop)

	if (card_mark < this_age) {
	    //
	    MAYBE_UPDATE_CARD_MIN_AGE_PER_POINTER( map, card_start, card_mark );
	}
    }									// while (p < sib->tospace.used_end)

    sib->tospace.swept_end = p;

    return TRUE;
}									// fun forward_rest_of_rw_pointers_sib

//
//
static void         forward_all_remaining_live_chunks                      (Heap* heap,  int oldest_agegroup_to_clean,  int max_swept_agegroup)   {
    //              =================================
    //
    // At this point we have forwarded (copied) into to-space
    // all the values referenced  from outside the from-spaces
    // of the agegroups we are cleaning.
    //
    // We now need to complete the process of copying all live
    // data from their from-spaces into their to-spaces by
    // recursively copying to to-space all from-space values
    // directly or indirectly ereferenced by the values already
    // forwarded.
    //
    // In essence, we have copied to to-space the root nodes of
    // various trees;  now we need to copy the rest of those trees.
    //
    // In principle we could do this via a simple recursive dagwalk,
    // but cleaning happens precisely when we do not have a lot
    // of spare ram, so to avoid using potentially a lot of stackspace
    // we treat to-space as a queue of values to process, and sweep
    // through that queue start-to-end.
    //
    // (Using a queue instead of a stack results in our doing a
    // breadth-first instead of depth-first traversal of the
    // live-value trees.  This may or may not be a slight speed
    // win due to improving cache coherence, depending upon who
    // you ask, but that isn't our primary motivation for doing
    // it this way.)
    //
    // This would be a really simple bit of code if to-space were a
    // single buffer, but is complicated because:
    //
    //   o We have a separate to-space for every agegroup being cleaned.
    //
    //   o Each from-space (except agegroup0, which we do not
    //     deal with in this file) is split into subbuffers for
    //
    //        RO_POINTERS_KIND
    //        RO_CONSCELL_KIND
    //        NONPTR_DATA_KIND
    //        RW_POINTERS_KIND
    //              CODE_KIND
    //
    //     Each of these subbuffers requires special handling --
    //     we divide data between them precisely to take
    //     advantage of their various special properties:
    //
    //         * NONPTR_DATA_KIND values (e.g., strings) contain no pointers,
    //           so we don't have to scan them at all.  This is why
    //           there is no NONPTR_DATA_KIND loop here.
    //
    //         * Pairs are always length 2, so we can avoid spending a
    //           tagword on each one, but cannot use our usual trick of
    //           changing the tagword to remember which ones we've forwarded.
    //
    //         * Codechunks we never actually copy at all, because it would
    //           take too long -- they are big and static and boring.
    //
    //         * Rw_Vectors and Refcells can be updated, so these are the only
    //           ones which can contain pointers into younger agegroups.
    //
    //   o As we run, we are appending additional
    //     values to the ends of these buffers.
    //
    // Consequently, instead of one simple loop we wind up repeatedly
    // iterating through the as-yet-unscanned parts of each of our
    // to-space buffers until we find nothing more to forward.
    //
    // Because there are few pointers from older to younger agegroups,
    // we work youngest-to-oldest through the agegroups we are cleaning,
    // doing as much forwarding as possible on each agegroup before
    // proceeding to the next-oldest one.


    Sibid max_sibid =  MAKE_MAX_SIBID( oldest_agegroup_to_clean );

    Bool   making_progress = TRUE;
    while (making_progress) {
	//
	making_progress = FALSE;

	for (int age = 0;   age < max_swept_agegroup;   ++age) {
	    //
	    Agegroup*  ag =  heap->agegroup[ age ];

            // Forward all live values referenced by unscanned parts
            // of to-space buffers for records, pairs and vectors:
	    //
	    making_progress |=   forward_rest_of_ro_pointers_sib(   ag, heap, RO_POINTERS_SIB, max_sibid );
	    making_progress |=   forward_rest_of_ro_pointers_sib(   ag, heap, RO_CONSCELL_SIB, max_sibid );
	    making_progress |=   forward_rest_of_rw_pointers_sib(   ag, heap, oldest_agegroup_to_clean );
	}
    }
}

//

//
//
static Val          forward_chunk                      (Heap* heap,  Sibid max_sibid,  Val v,  Sibid sibid)   {
    //              =============
    //
    // Forwarding a chunk involves
    //
    //  1) Copying the chunk's tagword and contents from
    //     the from-space sib buffer it is in into the
    //     appropriate to-space sib buffer.
    //
    //  2) Installing a forwarding pointer in the original
    //     chunk pointing to the new copy of it, and changing
    //     the original's tagword to flag it as forwarded.
    //
    // Return a pointer to the to-space copy.  (If 'v'
    // has already been forwarded, we simply return
    // the pre-existing	to-space copy.)


    Val		   tagword;		// Tagword of 'v'.
    Val_Sized_Unt  size_in_words;	// Number of words in 'v', not counting tagword.

    Sib*	   to_sib;		// We'll copy 'v' into to_sib's to-space.
    Val*  	   new_chunk;		// Newly-created to-space copy of 'v'.

    Val*  chunk =   PTR_CAST(Val*, v);


    switch (GET_KIND_FROM_SIBID( sibid )) {
	//
    case RO_POINTERS_KIND:
	{
	    tagword = chunk[ -1 ];

	    switch (GET_BTAG_FROM_TAGWORD( tagword )) {
		//
	    case RO_VECTOR_HEADER_BTAG:
	    case RW_VECTOR_HEADER_BTAG:												// NB: the vector *header* is read-only even when the *vector* is read-write.
		size_in_words = 2;
		break;

	    case FORWARDED_CHUNK_BTAG:
		//
		// This chunk has already been forwarded,
		// so return pre-existing to-space copy:
		//
		return PTR_CAST( Val, FOLLOW_FORWARDING_POINTER(chunk));							// FOLLOW_FORWARDING_POINTER				def in   src/c/h/heap.h

	    case PAIRS_AND_RECORDS_BTAG:
		//
		size_in_words =  GET_LENGTH_IN_WORDS_FROM_TAGWORD( tagword );
		break;

	    default:
		die ("Bad record b-tag %#x, chunk = %#x, tagword = %#x", GET_BTAG_FROM_TAGWORD( tagword ), chunk, tagword);
                exit(1);													// Cannot execute -- just to quiet gcc -Wall.
	    }

	    to_sib =  heap->agegroup[ GET_AGE_FROM_SIBID(sibid)-1 ]->sib[ RO_POINTERS_SIB ];

	    if (sib_chunk_is_senior( to_sib, chunk ))   to_sib = to_sib->sib_for_promoted_chunks;				// sib_chunk_is_senior				def in   src/c/h/heap.h
	}
	break;

    case RO_CONSCELL_KIND:
	// We store cons-cells (two-pointer records)
        // in the pair-sibs without tagwords:
	// That way they cost only two words of ram each,
	// instead of three.  (These are only only heap values
	// stored without tagwords.)
	//
	// One consequence of this is that we cannot use our usual trick of changing
	// the tagword B-tag to FORWARDED_CHUNK_BTAG to mark chunks which have been
	// forwarded;  instead we mark a forwarded pair by setting TAGWORD_ATAG (0x2)
	// on the forwarding link in pair[0]:
	{
	    // Check first word of pair to see if it
	    // has already been forwarded:
	    //
	    Val	chunk_0 = chunk[0];
	    //
	    if (IS_TAGWORD(chunk_0)) {
	        //
	        // Chunk has already been forwarded,
		// so return pre-existing to-space copy:
	        //
		return PTR_CAST( Val, FOLLOW_PAIRSPACE_FORWARDING_POINTER(chunk_0, chunk));					// FOLLOW_PAIRSPACE_FORWARDING_POINTER		def in    src/c/h/heap.h
	        //
	    } else {
	        //
	        // Forward the pair:
		//
		to_sib = heap->agegroup[ GET_AGE_FROM_SIBID(sibid)-1 ]->sib[ RO_CONSCELL_SIB ];

		if (sib_chunk_is_senior( to_sib, chunk ))   to_sib =  to_sib->sib_for_promoted_chunks;

		new_chunk =  to_sib->tospace.used_end;

		to_sib->tospace.used_end
		    +=
		    PAIR_SIZE_IN_WORDS;												// 2.	PAIR_SIZE_IN_WORDS			def in    src/c/h/runtime-base.h

		new_chunk[0] = chunk_0 ;
		new_chunk[1] = chunk[1];

		// Set up the forward pointer in the old pair,
		// setting the TAGWORD_ATAG bit (0x2) on the forwarding
		// pointer to distinguish it from a real pair value:
		//
		chunk[0] =  MAKE_PAIRSPACE_FORWARDING_POINTER( new_chunk );							// MAKE_PAIRSPACE_FORWARDING_POINTER		def in    src/c/h/heap.h

		return PTR_CAST( Val, new_chunk );
	    }
	}
	break;

    case NONPTR_DATA_KIND:
	{
	    tagword = chunk[ -1 ];
	    //
	    switch (GET_BTAG_FROM_TAGWORD( tagword )) {
		//
	    case FORWARDED_CHUNK_BTAG:
		//
		// Data block has already been forwarded;
		// return pre-existing to-space copy:
		//
		return PTR_CAST( Val, FOLLOW_FORWARDING_POINTER(chunk));

	    case FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG:
	        //
		size_in_words =  GET_LENGTH_IN_WORDS_FROM_TAGWORD( tagword );
		//
		to_sib = heap->agegroup[ GET_AGE_FROM_SIBID(sibid)-1 ]->sib[ NONPTR_DATA_SIB ];
		//
		if (sib_chunk_is_senior( to_sib, chunk ))   to_sib =  to_sib->sib_for_promoted_chunks;				// sib_chunk_is_senior				def in   src/c/h/heap.h
		//
		break;

	    case EIGHT_BYTE_ALIGNED_NONPOINTER_DATA_BTAG:
		//
		size_in_words =  GET_LENGTH_IN_WORDS_FROM_TAGWORD( tagword );
		//
		to_sib = heap->agegroup[ GET_AGE_FROM_SIBID(sibid)-1 ]->sib[ NONPTR_DATA_SIB ];
		//
		if (sib_chunk_is_senior( to_sib, chunk ))   to_sib =  to_sib->sib_for_promoted_chunks;				// sib_chunk_is_senior				def in   src/c/h/heap.h
		//
		#ifdef ALIGN_FLOAT64S
		#  ifdef CHECK_HEAP
			    if (((Punt) to_sib->tospace.used_end & WORD_BYTESIZE) == 0) {
				*to_sib->tospace.used_end = (Val)0;
				 to_sib->tospace.used_end++;
			    }
		#  else
			    to_sib->tospace.used_end = (Val*) (((Punt) to_sib->tospace.used_end) | WORD_BYTESIZE);
		#  endif
		#endif
		break;

	    default:
		die ("Bad nonpointer-data record b-tag %#x, chunk = %#x, tagword = %#x",  GET_BTAG_FROM_TAGWORD( tagword ), chunk, tagword);
                exit(1);														// Cannot execute -- just to quiet gcc -Wall.
	    }
        }
        break;

    case RW_POINTERS_KIND:
	{
	    tagword = chunk[-1];

	    switch (GET_BTAG_FROM_TAGWORD( tagword )) {
		//
	    case FORWARDED_CHUNK_BTAG:
		//
		// Refcell/rw_vector has already been forwarded;
		// return pre-existing to-space copy:
		//
		return PTR_CAST( Val, FOLLOW_FORWARDING_POINTER(chunk));	      // This chunk has already been forwarded.

	    case RW_VECTOR_DATA_BTAG:
		//
		size_in_words =  GET_LENGTH_IN_WORDS_FROM_TAGWORD( tagword );
		break;

	    case WEAK_POINTER_OR_SUSPENSION_BTAG:
		return forward_special_chunk( heap, max_sibid, chunk, sibid, tagword );

	    default:
		die ( "Fatal error: bad rw_vector b-tag %#x, chunk = %#x, tagword = %#x (= chunk[-1]) tag should be one of  %#x %#x %#x -- src/c/heapcleaner/heapclean-n-agegroups.c",
		      GET_BTAG_FROM_TAGWORD( tagword ),
		      chunk,
		      tagword,
		      FORWARDED_CHUNK_BTAG,
		      RW_VECTOR_DATA_BTAG,
		      WEAK_POINTER_OR_SUSPENSION_BTAG
		    );

                exit(1);											// Cannot execute -- just to quiet gcc -Wall.
	    }

	    to_sib = heap->agegroup[ GET_AGE_FROM_SIBID(sibid)-1 ]->sib[ RW_POINTERS_SIB ];

	    if (sib_chunk_is_senior( to_sib, chunk ))   to_sib =  to_sib->sib_for_promoted_chunks;		// sib_chunk_is_senior		def in   src/c/h/heap.h
        }
	break;

    case CODE_KIND:
	mark_hugechunk_as_live( heap, GET_AGE_FROM_SIBID(max_sibid), v, sibid );				// Actual forwarding/promotion (as appropriate) is done in a postpass -- see forward_promote_or_reclaim_all_hugechunks()
	return v;

    default:
	die("unknown chunk ilk %d @ %#x", GET_KIND_FROM_SIBID(sibid), chunk);
        exit(1);												// Cannot execute -- just to quiet gcc -Wall.
    }

    // Allocate and initialize a
    // to-space copy of the chunk:
    //
    new_chunk =  to_sib->tospace.used_end;

    to_sib->tospace.used_end +=   size_in_words + 1;								// + 1 for tagword.

    *new_chunk++ = tagword;

    ASSERT( sib->tospace.used_end <= sib->tospace.limit );

    COPYLOOP( chunk, new_chunk, size_in_words );								// COPYLOOP			def in   src/c/heapcleaner/copy-loop.h

    // Set up the forward pointer
    // and return the new chunk:
    //
    chunk[-1] =  FORWARDED_CHUNK_TAGWORD;
    chunk[ 0] =  (Val)(Punt) new_chunk;

    return PTR_CAST( Val, new_chunk);
}						// forward_chunk

//
//
static Hugechunk*   mark_hugechunk_as_live                  (Heap* heap,   int oldest_agegroup_to_clean,   Val codechunk,   Sibid sibid)   {
    //              ======================
    //
    // 'sibid' is the book_to_sibid__global entry for codechunk.
    //
    // Plain chunks are forwarded by copying them from a
    // sib buffer for one agegroup to the corresponding
    // sib buffer in the next-oldest agegroup.
    //
    // Hugechunks are (by definition) too large to copy
    // during heapcleaning, so promoting them is a matter
    // of bookkeeping rather than copying.
    //  
    // We do the actual work of promoting and forwarding
    // hugechunks in a separate postpass, implemented in
    //  
    //     forward_promote_or_reclaim_all_hugechunks ()
    //
    // Here we merely set the hugechunk's state to record
    // whether it should be forwarded or promoted.  (Any
    // hugechunk not forwarded or promoted will be reclaimed.)
    //
    // Return the Hugechunk record for 'codechunk'.


    INCREMENT_HUGECHUNK2_COUNT;													// INCREMENT_HUGECHUNK2_COUNT		def at top of file.

    Hugechunk_Quire* hq;													// Hugechunk_Quire			def in    src/c/h/heap.h
    {
	int  book;
	for (book =  GET_BOOK_CONTAINING_POINTEE( codechunk );
	    //
            !SIBID_ID_IS_BIGCHUNK_RECORD( sibid );
	    //
            sibid =  book_to_sibid__global[ --book ]
        );

	hq =  (Hugechunk_Quire*) ADDRESS_OF_BOOK( book );
    }
 
    Hugechunk*															// Hugechunk				def in    src/c/h/heap.h
        //
	hc = get_hugechunk_holding_pointee( hq, codechunk );									// get_hugechunk_holding_pointee	def in    src/c/h/heap.h

    if (hc->age <= oldest_agegroup_to_clean
        &&
        HUGECHUNK_IS_IN_FROMSPACE( hc )
    ){
	//
        INCREMENT_HUGECHUNK3_COUNT;												// INCREMENT_HUGECHUNK2_COUNT		def at top of file.

	// Forward the hugechunk.
        // Note that chunks in the oldest agegroup
	// will always be YOUNG, thus will never be promoted:									// JUNIOR_HUGECHUNK_WAITING_TO_BE_FORWARDED		def in    src/c/h/heap.h
	//
	if (hc->hugechunk_state == JUNIOR_HUGECHUNK)  hc->hugechunk_state = JUNIOR_HUGECHUNK_WAITING_TO_BE_FORWARDED;		// This is the only place we set state to JUNIOR_HUGECHUNK_WAITING_TO_BE_FORWARDED
	else			                      hc->hugechunk_state = SENIOR_HUGECHUNK_WAITING_TO_BE_PROMOTED;		// This is the only place we set state to SENIOR_HUGECHUNK_WAITING_TO_BE_PROMOTED
    }

    return  hc;
}

//
//
static Val          forward_special_chunk   (
    //              =====================
    //
    Heap*	heap,
    Sibid	max_sibid,
    Val*	chunk,
    Sibid	sibid,
    Val		tagword
) {
    // Forward a "special chunk". (A suspension or weakref.)
    //
    // This is trivial except in the case of weakrefs to
    // live data, where we must null out the weakref if
    // the referenced value gets garbage-collected this run.
    //
    // We do this by making a list of all weakrefs processed
    // and then after heapcleaning is complete checking all
    // weakrefs on the list.  Any which point to garbage need
    // to be nulled out by null_out_newly_dead_weakrefs ().									// null_out_newly_dead_weakrefs		def in   src/c/heapcleaner/heapcleaner-stuff.c
    //
    Agegroup*	ag = heap->agegroup[ GET_AGE_FROM_SIBID(sibid)-1 ];
    Sib*	sib = ag->sib[ RW_POINTERS_SIB ];
    Val*	new_chunk;

    if (sib_chunk_is_senior( sib, chunk ))   sib = sib->sib_for_promoted_chunks;						// sib_chunk_is_senior		def in   src/c/h/heap.h

    // Allocate the new chunk:
    //
    new_chunk = sib->tospace.used_end;
    sib->tospace.used_end += SPECIAL_CHUNK_SIZE_IN_WORDS;									// All specials are two words.

    switch (GET_LENGTH_IN_WORDS_FROM_TAGWORD(tagword)) {									// We abuse the 'length' field in specials to carry extra type information.
        //															// Since all specials are two words long, this causes no problem.
    case EVALUATED_LAZY_SUSPENSION_CTAG:
    case UNEVALUATED_LAZY_SUSPENSION_CTAG:
    case NULLED_WEAK_POINTER_CTAG:
	*new_chunk++ = tagword;
	*new_chunk   = *chunk;
	break;

    case WEAK_POINTER_CTAG:
        {
	    Val	v = *chunk;

									    #ifdef DEBUG_WEAKREFS
										debug_say ("heapclean_n_agegroups: weak [%#x ==> %#x] --> %#x", chunk, new_chunk+1, v);
									    #endif

	    if (! IS_POINTER(v)) {
										#ifdef DEBUG_WEAKREFS
										    debug_say (" unboxed\n");
										#endif

	        // Weak references to unboxed chunks are never nullified:
                //
		new_chunk[0] =  WEAKREF_TAGWORD;
		new_chunk[1] =  v;

		++new_chunk;

	    } else {

		Sibid	sibid = SIBID_FOR_POINTER( book_to_sibid__global, v );
		Val*	vp = PTR_CAST(Val*, v);
		Val	tagword;

		if (! SIBID_IS_IN_FROMSPACE( sibid, max_sibid )) {

		    // Reference to an older chunk:
                    //
										    #ifdef DEBUG_WEAKREFS
											debug_say (" old chunk\n");
										    #endif
		    new_chunk[0] =  WEAKREF_TAGWORD;
		    new_chunk[1] =  v;

		    ++new_chunk;

		} else {

		    //
		    switch (GET_KIND_FROM_SIBID( sibid )) {
		        //
		    case RO_POINTERS_KIND:
		    case NONPTR_DATA_KIND:
		    case RW_POINTERS_KIND:
		        //
			tagword = vp[-1];
		        //
			if (tagword == FORWARDED_CHUNK_TAGWORD) {
			    //
			    // Reference to a chunk that has already been forwarded.
			    // NOTE: we have to put the pointer to the non-forwarded
			    // copy of the chunk (i.e, v) into the to-space copy
			    // of the weak pointer, since the heapcleaner has the invariant
			    // that it never sees to-space pointers during sweeping.

											    #ifdef DEBUG_WEAKREFS
												debug_say (" already forwarded to %#x\n", FOLLOW_FORWARDING_POINTER(vp));
											    #endif
			    new_chunk[0] =  WEAKREF_TAGWORD;
			    new_chunk[1] =  v;

			    ++new_chunk;

			} else {

			    // This is the important case: We are copying a weakref
			    // of an agegroup0 value.  That agegroup0 value might get
			    // get garbage-collected this pass; if it does, we must null
			    // out the weakref.
			    //
			    // To do this efficiently, as we copy such weakrefs from
			    // agegroup0 into agegroup1 we chain them togther via
			    // their tagword fields with the root pointer kept
			    // in ag1->heap->weakrefs_forwarded_during_heapcleaning.
			    //
			    // At the end of heapcleaning we will consume this chain of
			    // weakrefs in null_out_newly_dead_weakrefs() where					// null_out_newly_dead_weakrefs	is from   src/c/heapcleaner/heapcleaner-stuff.c
			    // we will null out any newly dead weakrefs and then
			    // replace the chainlinks with valid tagwords -- either
			    // WEAKREF_TAGWORD or NULLED_WEAKREF_TAGWORD,
			    // as appropriate, thus erasing our weakref chain and
			    // restoring sanity.
			    //
			    // We mark the chunk reference field in the forwarded copy
			    // to make it look like an Tagged_Int so that the to-space
			    // sweeper does not follow the weak reference.

											    #ifdef DEBUG_WEAKREFS
												debug_say (" forward (start = %#x)\n", vp);
											    #endif

			    new_chunk[0] =  MARK_POINTER(PTR_CAST( Val, ag->heap->weakrefs_forwarded_during_heapcleaning));
			    new_chunk[1] =  MARK_POINTER(vp);

			    ag->heap->weakrefs_forwarded_during_heapcleaning =  new_chunk;

			    ++new_chunk;
			}
			break;

		    case RO_CONSCELL_KIND:
		        //
			if (IS_TAGWORD(tagword = vp[0])) {
			    //
			    // Reference to a pair that has already been forwarded.
			    // NOTE: we have to put the pointer to the non-forwarded
			    // copy of the pair (i.e, v) into the to-space copy
			    // of the weak pointer, since the heapcleaner has the invariant
			    // that it never sees to-space pointers during sweeping.

											    #ifdef DEBUG_WEAKREFS
												debug_say (" (pair) already forwarded to %#x\n", FOLLOW_PAIRSPACE_FORWARDING_POINTER(tagword, vp));
											    #endif

			    new_chunk[0] =  WEAKREF_TAGWORD;
			    new_chunk[1] =  v;

			    ++new_chunk;

			} else {

			    new_chunk[0] = MARK_POINTER(PTR_CAST( Val, ag->heap->weakrefs_forwarded_during_heapcleaning));
			    new_chunk[1] = MARK_POINTER(vp);

			    ag->heap->weakrefs_forwarded_during_heapcleaning =  new_chunk;

			    ++new_chunk;
			}
			break;

		    case CODE_KIND:
			die ("weak big chunk");
                        exit(1);								// Cannot execute -- just to quiet gcc -Wall.
			break;
		    }
		}
	    }
	}
        break;

    default:
	die ( "strange/unexpected special chunk @ %#x; tagword = %#x\n", chunk, tagword );
        exit(1);										// Cannot execute -- just to quiet gcc -Wall.
    }												// switch (GET_LENGTH_IN_WORDS_FROM_TAGWORD(tagword))

    chunk[-1] =  FORWARDED_CHUNK_TAGWORD;
    chunk[ 0] =  (Val) (Punt) new_chunk;

    return PTR_CAST( Val, new_chunk);
}								// fun forward_special_chunk

//
//
static void         trim_heap   (Task* task,  int oldest_agegroup_to_clean)   {
    //              =========
    // 
    // After a major collection, trim any sib buffers that are over their maximum
    // size in space-allocated, but under their maximum size in space-used.

    Heap*  heap  =  task->heap;

												// unlimited_heap_is_enabled__global defaults to FALSE in	src/c/main/runtime-main.c 
    if (unlimited_heap_is_enabled__global)   return;						// unlimited_heap_is_enabled__global can be set TRUE via --runtime-unlimited-heap commandline arg -- see   src/c/main/runtime-options.c
												// unlimited_heap_is_enabled__global can be set via _lib7_cleaner_control -- see   src/c/lib/heap/heapcleaner-control.c
    Val_Sized_Unt   min_bytesize;
    Val_Sized_Unt   new_bytesize;

    for (int a = 0;  a < oldest_agegroup_to_clean;  a++) {
	//
	Agegroup* age = heap->agegroup[ a ];

	for (int s = 0;  s < MAX_PLAIN_SIBS;  s++) {
	    //
	    Sib* sib = age->sib[ s ];

	    if (sib_is_active(sib)												// sib_is_active		def in    src/c/h/heap.h
            &&  sib->tospace.bytesize > sib->soft_max_bytesize
            ){

		min_bytesize
		    =
		    (a == 0)
		    ? task->heap_allocation_buffer_bytesize
		    : heap->agegroup[ a-1 ]->sib[ s ]->tospace.bytesize;

		min_bytesize +=  sib_space_used_in_bytes( sib )									// sib_space_used_in_bytes	def in    src/c/h/heap.h
                                 +
			         sib->requested_extra_free_bytes;

		if (min_bytesize < sib->soft_max_bytesize) {
		    new_bytesize = sib->soft_max_bytesize;

		} else {

		    new_bytesize = BOOKROUNDED_BYTESIZE( min_bytesize );

		    // The calculation of minSz here may
                    // return something bigger than
		    // what establish_all_necessary_empty_tospace_sib_buffers computed!
		    //
		    if (new_bytesize > sib->tospace.bytesize) {
			new_bytesize = sib->tospace.bytesize;
		    }
		}
		sib->tospace.bytesize = new_bytesize;

		sib->tospace.limit =  (Val*) ((Punt)sib->tospace.start + sib->tospace.bytesize);
	    }
	}
    }
}									// fun trim_heap


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
