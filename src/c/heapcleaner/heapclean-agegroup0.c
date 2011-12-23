// heapclean-agegroup0.c
//
// This file contains the code for doing "minor"
// cleanings ("garbage collections") -- those
// which recover unused memory only in the agegroup0
// buffer.
//
// For more on the Mythryl heap datastructures see
//
//     src/A.HEAP.OVERVIEW

/*
###            "It goes against the grain of modern
###             education to teach children to program.
###
###            "What fun is there in making plans,
###             acquiring discipline in organizing thoughts,
###             devoting attention to detail, and
###             learning to be self-critical?"
###
###                               -- Alan Perlis
*/

#include "../mythryl-config.h"

#include <stdio.h>

#include "runtime-base.h"
#include "runtime-configuration.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "coarse-inter-agegroup-pointers-map.h"
#include "heap.h"
#include "heap-tags.h"
#include "copy-loop.h"


// Cleaner statistics:
//
extern long	update_count__global;		// update_count__global				def in    src/c/heapcleaner/heapclean-n-agegroups.c
extern long	total_bytes_allocated__global;	// total_bytes_allocated__global		def in    src/c/heapcleaner/heapclean-n-agegroups.c
extern long	total_bytes_copied__global;	// total_bytes_allocated__global		def in    src/c/heapcleaner/heapclean-n-agegroups.c

// heap_changelog operations:
//
#define HEAP_CHANGELOG_NIL	HEAP_VOID
#define HEAP_CHANGELOG_HEAD(p)	GET_TUPLE_SLOT_AS_PTR( Val*, p, 0 )
#define HEAP_CHANGELOG_TAIL(p)	GET_TUPLE_SLOT_AS_VAL(       p, 1 )

//
static void   process_task_heap_changelog (Task* task,  Heap* heap);
static void   sweep_agegroup_1_tospace	(Agegroup* agegroup_1, Task* task);	// 'task' arg is purely for debugging, can be deleted for production use.
//
static        Val    forward_agegroup0_chunk_to_agegroup_1	(Agegroup* agegroup_1,  Val v, Task* task, int caller);	// 'task' arg is only for debugging, can be dropped in production code.
static        Val    forward_special_chunk			(Agegroup* agegroup_1,  Val* chunk,  Val tagword);


#ifdef VERBOSE
    extern char* sib_name__global [];			// sib_name__global	def in   src/c/heapcleaner/heapclean-n-agegroups.c
#endif

static inline void   forward_if_in_agegroup0   (Sibid* book2sibid,  Agegroup* g1,  Val *p, Task* task) {	// 'task' arg is only for debugging, can be dropped in production code.
    //               =======================
    //
    // Forward *p if it is in agegroup0:

    Val	w = *(p);
    //
    if (IS_POINTER(w)) {
	//
	Sibid  sibid =  SIBID_FOR_POINTER( book2sibid, w );

	if (sibid == NEWSPACE_SIBID)   *p =  forward_agegroup0_chunk_to_agegroup_1( g1, w, task, 0 );
    }
}


// This fun is called (only) from:
//
//     src/c/heapcleaner/call-heapcleaner.c
//
void   heapclean_agegroup0   (Task* task,  Val** roots) {
    // ===================
    //
    // Do "garbage collection" on just agegroup0.
    //
    // NB: If we have multiple pthreads running,
    // each has its own agegroup0, but we process
    // all of those during this call, by virtue
    // of being passed all the roots from all the
    // running pthreads. 

    Heap*      heap =  task->heap;
    Agegroup*  age1 =  heap->agegroup[0];

    Punt	age1_tospace_top   [ MAX_PLAIN_ILKS ];

    long bytes_allocated = (Punt) task->heap_allocation_pointer
                         - (Punt) heap->agegroup0_buffer;

    INCREASE_BIGCOUNTER( &heap->total_bytes_allocated, bytes_allocated );

    for (int i = 0;  i < MAX_PLAIN_ILKS;  i++) {
	//
	age1_tospace_top[i]
	    =
	    (Punt)
		//
		age1->sib[ i ]->next_tospace_word_to_allocate;
    }


    #ifdef VERBOSE
	debug_say ("Agegroup 1 before cleaning agegroup0:\n");
	//
	for (int i = 0;  i < MAX_PLAIN_ILKS;  i++) {
	    //
	    debug_say ("  %s: to-space bottom = %#x, end of fromspace oldstuff = %#x, next_tospace_word_to_allocate = %#x\n",
		//
	        sib_name__global[ i+1 ],
		//
                age1->sib[ i ]->tospace,
	        age1->sib[ i ]->end_of_fromspace_oldstuff,
                age1->sib[ i ]->next_tospace_word_to_allocate
            );
	}
    #endif

    // Scan the standard roots:
    //
    {   Sibid*  b2s = book_to_sibid__global;									// Cache global locally for speed.   book_to_sibid__global	def in    src/c/heapcleaner/heapcleaner-initialization.c
	Val*    rp;
	while ((rp = *roots++) != NULL) {
	    //
	    forward_if_in_agegroup0( b2s, age1, rp, task );
	}
    }

    // Scan the store log:
    //
    #if NEED_PTHREAD_SUPPORT
    {
	for (int i = 0;  i < MAX_PTHREADS;  i++) {									// Potentially need to process one heap storelog per pthread.
	    //
	    Pthread* pthread =  pthread_table__global[ i ];
	    //
	    Task*   task     =  pthread->task;
	    //
	    if (pthread->mode != PTHREAD_IS_VOID) {
		//
		process_task_heap_changelog( task, heap );
	    }
	}
    }
    #else // Same as }else{ clause above:
	//
	process_task_heap_changelog( task, heap );									// Just one heap storelog to process.
    #endif

    // Sweep the to-space for agegroup 1:
    //
    sweep_agegroup_1_tospace( age1, task );
    ++heap->agegroup0_cleanings_done;

    null_out_newly_dead_weak_pointers( heap );										// null_out_newly_dead_weak_pointers		def in    src/c/heapcleaner/heapcleaner-stuff.c

    #ifdef VERBOSE
	debug_say ("Agegroup 1 after MinorGC:\n");
	for (int i = 0;  i < MAX_PLAIN_ILKS;  i++) {
	  debug_say ("  %s: base = %#x, oldTop = %#x, next_tospace_word_to_allocate = %#x\n",
	    sib_name__global[i+1], age1->sib[i]->tospace,
	    age1->sib[i]->oldTop, age1->sib[i]->next_tospace_word_to_allocate);
	}
    #endif

    // Cleaner statistics stuff:
    {
	long bytes_copied = 0;

	for (int i = 0;  i < MAX_PLAIN_ILKS;  i++) {
	    //
	    int bytes = (Val_Sized_Unt) age1->sib[ i ]->next_tospace_word_to_allocate - age1_tospace_top[ i ];

	    bytes_copied += bytes;

	    INCREASE_BIGCOUNTER( &heap->total_bytes_copied_to_sib[ 0 ][ i ], bytes );
	}

	total_bytes_allocated__global  +=  bytes_allocated;				// Never used otherwise.
	total_bytes_copied__global     +=  bytes_copied;					// Never used otherwise.

	#ifdef XXX
	    debug_say ("Minor GC: %d/%d (%5.2f%%) bytes copied; %d updates\n",
	    bytes_copied, bytes_allocated,
	    (bytes_allocated ? (double)(100*bytes_copied)/(double)bytes_allocated : 0.0),
	    update_count__global - nUpdates);
	#endif
    }


    #ifdef CHECK_HEAP
        check_heap( heap, 1 );								// check_heap		def in    src/c/heapcleaner/check-heap.c
    #endif

}											// fun heapclean_agegroup0


static int   get_age_of_codechunk   (Val codechunk) {
    //       ====================

    Sibid* b2s =  book_to_sibid__global;							// Cache global locally for speed.   book_to_sibid__global	def in    src/c/heapcleaner/heapcleaner-initialization.c

    Sibid dst_sibid =  SIBID_FOR_POINTER(b2s, codechunk );				// Get the Sibid tag for the ram-book containing the codechunk.

    int  book;
    for (book = GET_BOOK_CONTAINING_POINTEE( codechunk );
	//
	!SIBID_ID_IS_BIGCHUNK_RECORD( dst_sibid );
	//
	dst_sibid = b2s[ --book ]
    );

    Hugechunk_Region*  hugechunk_region
	=
	(Hugechunk_Region*)   ADDRESS_OF_BOOK( book );

    Hugechunk* dp =  get_hugechunk_holding_pointee( hugechunk_region, codechunk );		// get_hugechunk_holding_pointee		def in    src/c/h/heap.h
    return     dp->age;
}


//
static void   process_task_heap_changelog   (Task* task, Heap* heap) {
    //        ===========================
    // 
    // As tasks run, they note all stores into refcells
    // and vectors in the 'heap_changelog', a lisp-style list
    // of "CONS cells" -- (val,next) pointer-pairs.
    // 
    // We need this done because such stores into the heap
    // can introduce pointers from one agegroup into a
    // younger agegroup, which we need to take into account
    // when doing partial cleanings ("garbage collections").
    //
    // Our job here is to promote to agegroup 1 all agegroup0
    // values referenced by a refcell/vectorslot in the heap_changelog.

    Val this_heap_changelog_cell =  task->heap_changelog; 
    if (this_heap_changelog_cell == HEAP_CHANGELOG_NIL)   return;			// Abort quickly if no work to do.

    int updates        = 0;								// Cleaner statistics.
    Agegroup* age1     =  heap->agegroup[ 0 ];						// Cache heap entry for speed.
    Sibid* b2s         =  book_to_sibid__global;						// Cache global locally for speed.   book_to_sibid__global	def in    src/c/heapcleaner/heapcleaner-initialization.c

    while (this_heap_changelog_cell != HEAP_CHANGELOG_NIL) {				// Over all entries in the heap_changelog.
	//
	++updates;									// Cleaner statistics.

	Val* pointer   	         =  HEAP_CHANGELOG_HEAD( this_heap_changelog_cell );	// Get pointer to next updated refcell/vector slot to process.
	this_heap_changelog_cell =  HEAP_CHANGELOG_TAIL( this_heap_changelog_cell );	// Step to next cell in heap_changelog list.

	Val pointee = *pointer;								// Get contents of updated refcell/vectorslot.

	if (!IS_POINTER( pointee ))   continue;						// Ignore refcells and vectorslots containing Tagged_Int values.

	Sibid src_sibid =  SIBID_FOR_POINTER(b2s, pointer );				// Get the Sibid tag for the ram-book containing the refcell/vectorslot.	Sibid  def in    src/c/h/sibid.h

	if (src_sibid == NEWSPACE_SIBID)    continue;					// Ignore updates to new-space      refcells and vectorslots.
	if (BOOK_IS_UNMAPPED( src_sibid ))  continue;					// Ignore updates to runtime-global refcells and vectorslots, which are handled elsewhere.

	Sibid dst_sibid =  SIBID_FOR_POINTER(b2s, pointee );				// Get the Sibid tag for the ram-book containing the value referenced by the refcell/vectorslot.
	//
	int src_age =  GET_AGE_FROM_SIBID( src_sibid );					// agegroup of the updated refcell/vectorslot.
	int dst_age =  GET_AGE_FROM_SIBID( dst_sibid );					// agegroup of the chunk that the refcell/vectorslot points to.

	if (!SIBID_KIND_IS_CODE( dst_sibid )) {
	    //
	    if (dst_age == AGEGROUP0) {
		//
		*pointer =  forward_agegroup0_chunk_to_agegroup_1( age1, pointee,task, 1);	// Promote pointee to agegroup 1.
		dst_age = 1;								// Remember pointee now has age 1, not 0.
		//
	    }

	} else {									// Refcell/vector slot is pointing to code.	

	    if (dst_age >= src_age)   continue;

            dst_age =  get_age_of_codechunk( pointee );
	}

	// Maybe update min_age value for
	// the card containing 'pointer':
	//
	if (src_age > dst_age) {
	    //
	    MAYBE_UPDATE_CARD_MIN_AGE_PER_POINTER(					// MAYBE_UPDATE_CARD_MIN_AGE_PER_POINTER	def in    src/c/h/coarse-inter-agegroup-pointers-map.h
		//
		heap->agegroup[ src_age-1 ]->coarse_inter_agegroup_pointers_map,
		pointer,
		dst_age
	    );
	}
    }

    update_count__global += updates;							// Cleaner statistics.  Apparently never used.

    task->heap_changelog =  HEAP_CHANGELOG_NIL;						// We're done with heap_changelog so clear it.

}											// fun process_task_heap_changelog

//
inline static Bool   sweep_agegroup_1_sib_tospace   (Agegroup* ag1,  int ilk, Task* task)   {	// Called only from sweep_agegroup_1_tospace (below).
    //               ============================					// 'task' arg is only for debugging, can be dropped in production code.
    //
    Sibid* b2s =  book_to_sibid__global;							// Cache global locally for speed.   book_to_sibid__global	def in    src/c/heapcleaner/heapcleaner-initialization.c
    Sib*   sib =  ag1->sib[ ilk ];							// Find sib to scan.

    Bool   progress =  FALSE;

    Val* p = sib->next_word_to_sweep_in_tospace;					// Pick up scanning this sib where we last left off.
    if  (p < sib->next_tospace_word_to_allocate) {
	//
	progress = TRUE;

        Val*q;
	do {
	    for (q = sib->next_tospace_word_to_allocate;  p < q;  p++) {		// Check all words in buffer.
		//
	      forward_if_in_agegroup0(b2s, ag1, p, task);				// If current agegroup 1 word points to an agegroup0 value, copy that value into agegroup 1.
	    }
	} while (q != sib->next_tospace_word_to_allocate);				// If the above loop has added stuff to our agegroup 1 buffer, process that stuff too.

	sib->next_word_to_sweep_in_tospace = q;						// Remember where we left off. (Current end of buffer.)
    }

    return progress;
}
//
static void   sweep_agegroup_1_tospace   (Agegroup* ag1, Task* task)   {		// 'task' arg is only for debugging, can be removed in production use.
    //        ========================
    //
    // Search the agegroup 1 to-space buffers for references
    // to values in agegroup0.  Copy all such agegroup0 values	
    // to agegroup1, and then search them for more such references
    // to agegroup0 values, continuing until progress ceases.
    //
    // Note that since there are no younger chunks,
    // we don't have to do anything special for the
    // vector sib.

    Bool making_progress = TRUE;

    while (making_progress) {
	//
	making_progress = FALSE;
	//
	making_progress |=  sweep_agegroup_1_sib_tospace(ag1, RECORD_ILK, task );
	making_progress |=  sweep_agegroup_1_sib_tospace(ag1,   PAIR_ILK, task );
	making_progress |=  sweep_agegroup_1_sib_tospace(ag1, VECTOR_ILK, task );
    };
}										// fun sweep_agegroup_1_tospace.


//
static Val   forward_agegroup0_chunk_to_agegroup_1   (Agegroup* ag1,  Val v, Task* task, int caller)   {	// 'task' arg is only for debugging, can be removed in production use.
    //       =====================================
    // 
    // Forward pair/record/vector/string 'v' from agegroup0 to agegroup 1.
    // This involves:
    // 
    //   o Duplicating v in the appropriate agegroup 1 to-space buffer.
    //   o Setting v's tagword to FORWARDED_CHUNK_TAGWORD.
    //   o Setting v's first slot to point to the duplicate.
    //   o Returning the duplicate.

    Val*           new_chunk;
    Val_Sized_Unt  len;
    Sib*           sib;

    Val*  chunk =   PTR_CAST(Val*, v);
    Val tagword =   chunk[-1];

    switch (GET_BTAG_FROM_TAGWORD( tagword )) {
	//
    case PAIRS_AND_RECORDS_BTAG:
	len = GET_LENGTH_IN_WORDS_FROM_TAGWORD( tagword );

	#ifdef NO_PAIR_STRIP							// Nowhere else mentioned in codebase.
	    sib = ag1->sib[RECORD_ILK];
	#else
	    if (len != 2) {
		sib = ag1->sib[ RECORD_ILK ];
	    } else {
		//
		sib = ag1->sib[ PAIR_ILK ];
		new_chunk = sib->next_tospace_word_to_allocate;
		sib->next_tospace_word_to_allocate += 2;
		new_chunk[0] = chunk[0];
		new_chunk[1] = chunk[1];

		// Set up the forward pointer in the old pair:
		//
		chunk[-1] = FORWARDED_CHUNK_TAGWORD;
		chunk[0] = (Val)(Punt)new_chunk;
		return PTR_CAST( Val, new_chunk );
	    }
        #endif
	break;

    case RO_VECTOR_HEADER_BTAG:
    case RW_VECTOR_HEADER_BTAG:
	//
	len = 2;
	sib = ag1->sib[ RECORD_ILK ];
	break;

    case RW_VECTOR_DATA_BTAG:
	//
	len = GET_LENGTH_IN_WORDS_FROM_TAGWORD( tagword );
	//
	sib = ag1->sib[ VECTOR_ILK ];
	break;

    case FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG:
	//
	len = GET_LENGTH_IN_WORDS_FROM_TAGWORD( tagword );
	//
	sib = ag1->sib[ STRING_ILK ];
	break;

    case EIGHT_BYTE_ALIGNED_NONPOINTER_DATA_BTAG:
	//
	len = GET_LENGTH_IN_WORDS_FROM_TAGWORD( tagword );
	//
	sib = ag1->sib[ STRING_ILK ];
	//
	#ifdef ALIGN_FLOAT64S
	#  ifdef CHECK_HEAP
		if (((Punt)sib->next_tospace_word_to_allocate & WORD_BYTESIZE) == 0) {
		    //
		    *sib->next_tospace_word_to_allocate++ = (Val) 0;
		}
	#  else
		sib->next_tospace_word_to_allocate = (Val*) (((Punt)sib->next_tospace_word_to_allocate) | WORD_BYTESIZE);
	#  endif
	#endif
	break;

    case WEAK_POINTER_OR_SUSPENSION_BTAG:
	//
	return forward_special_chunk ( ag1, chunk, tagword );

    case FORWARDED_CHUNK_BTAG:
	//
	return PTR_CAST( Val, FOLLOW_FWDCHUNK(chunk));

    default:
	log_if("bad chunk tag %d, chunk = %#x, tagword = %#x   -- forward_agegroup0_chunk_to_agegroup_1() in src/c/heapcleaner/heapclean-agegroup0.c", GET_BTAG_FROM_TAGWORD(tagword), chunk, tagword);
	log_if("forward_agegroup0_chunk_to_agegroup_1 was called by %s", caller ? "process_task_heap_changelog" : "forward_if_in_agegroup0");
	log_task(task,"forward_agegroup0_chunk_to_agegroup_1/default");
	die ("bad chunk tag %d, chunk = %#x, tagword = %#x   -- forward_agegroup0_chunk_to_agegroup_1() in src/c/heapcleaner/heapclean-agegroup0.c", GET_BTAG_FROM_TAGWORD(tagword), chunk, tagword);
	exit(1);									// Cannot execute -- just to quiet gcc -Wall.
    }

    // Allocate and initialize a to-space copy of the chunk:
    //
    new_chunk = sib->next_tospace_word_to_allocate;
    sib->next_tospace_word_to_allocate += (len + 1);
    *new_chunk++ = tagword;
    ASSERT( sib->next_tospace_word_to_allocate <= sib->tospace_limit );

    COPYLOOP(chunk, new_chunk, len);

    // Set up the forward pointer, and return the new chunk:
    //
    chunk[-1] =  FORWARDED_CHUNK_TAGWORD;
    chunk[ 0] =  (Val) (Punt) new_chunk;

    return PTR_CAST( Val, new_chunk );
}										// fun forward_agegroup0_chunk_to_agegroup_1

//
static Val   forward_special_chunk   (Agegroup* ag1,  Val* chunk,   Val tagword)   {
    //       =====================
    // 
    // Forward a special chunk (suspension or weak pointer).

    Sib*  sib =  ag1->sib[ VECTOR_ILK ];

    Val*  new_chunk = sib->next_tospace_word_to_allocate;

    sib->next_tospace_word_to_allocate += SPECIAL_CHUNK_SIZE_IN_WORDS;			// All specials are two words.

    switch (GET_LENGTH_IN_WORDS_FROM_TAGWORD( tagword )) {
        //
    case EVALUATED_LAZY_SUSPENSION_CTAG:
    case UNEVALUATED_LAZY_SUSPENSION_CTAG:
        //
	*new_chunk++ = tagword;
	*new_chunk = *chunk;
	break;

    case WEAK_POINTER_CTAG:
        {
      	    //
	    Val	v = *chunk;

	    #ifdef DEBUG_WEAK_PTRS
		debug_say ("MinorGC: weak [%#x ==> %#x] --> %#x", chunk, new_chunk+1, v);
	    #endif

	    if (! IS_POINTER( v )) {

		#ifdef DEBUG_WEAK_PTRS
		debug_say (" unboxed\n");
		#endif

	        // Weak references to unboxed chunks are never nullified:
                //
		*new_chunk++ = WEAK_POINTER_TAGWORD;
		*new_chunk = v;

	    } else {

		Sibid sibid =  SIBID_FOR_POINTER( book_to_sibid__global, v );
		Val*  vp    =  PTR_CAST( Val*, v );

		if (sibid == NEWSPACE_SIBID) {
		    //
		    if (vp[-1] == FORWARDED_CHUNK_TAGWORD) {
		        //
			// Reference to a chunk that has already been forwarded.
			// Note that we have to put the pointer to the non-forwarded
			// copy of the chunk (i.e, v) into the to-space copy
			// of the weak pointer, since the cleaner has the invariant
			// that it never sees to-space pointers during sweeping.

			#ifdef DEBUG_WEAK_PTRS
			    debug_say (" already forwarded to %#x\n", PTR_CAST( Val, FOLLOW_FWDCHUNK(vp)));
			#endif

			*new_chunk++ = WEAK_POINTER_TAGWORD;
			*new_chunk = v;

		    } else {

			// The forwarded version of weak chunks are threaded
			// via their tagword fields.  We mark the chunk
			// reference field to make it look like an Tagged_Int
			// so that the to-space sweeper does not follow
			// the weak reference.

			#ifdef DEBUG_WEAK_PTRS
			    debug_say (" forward\n");
			#endif

			*new_chunk = MARK_POINTER(PTR_CAST( Val, ag1->heap->weak_pointers_forwarded_during_cleaning));

			ag1->heap->weak_pointers_forwarded_during_cleaning = new_chunk++;

			*new_chunk = MARK_POINTER(vp);
		    }

		} else {

		    // Reference to an older chunk.
		    //
		    #ifdef DEBUG_WEAK_PTRS
		        debug_say (" old chunk\n");
		    #endif

		    *new_chunk++ = WEAK_POINTER_TAGWORD;
		    *new_chunk = v;
		}
	    }
	}
	break;

    case NULLED_WEAK_POINTER_CTAG:					// Shouldn't happen in agegroup0.
    default:
	die (
            "strange/unexpected special chunk @ %#x; tagword = %#x\n",
            chunk, tagword
	);
    }								// switch (GET_LENGTH_IN_WORDS_FROM_TAGWORD(tagword))

    chunk[-1] =  FORWARDED_CHUNK_TAGWORD;
    chunk[ 0] =  (Val) (Punt) new_chunk;

    return   PTR_CAST( Val, new_chunk );
}								// fun forward_special_chunk


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
# outline-regexp: "[a-z]"			 		 	#
# End:									 #
##########################################################################
*/
