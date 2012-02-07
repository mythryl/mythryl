// datastructure-pickler-cleaner.c
//
// This is the cleaner for compacting a pickled datastructure.
//
// For background see:  src/A.DATASTRUCTURE-PICKLING.OVERVIEW
//
// NOTE:  The extraction of literals could cause a space overflow.	XXX BUGGO FIXME

#include "../mythryl-config.h"

#include <stdio.h>
#include "runtime-base.h"
#include "runtime-configuration.h"
#include "runtime-values.h"
#include "get-quire-from-os.h"
#include "coarse-inter-agegroup-pointers-map.h"
#include "heap.h"
#include "heap-tags.h"
#include "copy-loop.h"
#include "runtime-timer.h"
#include "runtime-heap-image.h"
#include "datastructure-pickler.h"
#include "address-hashtable.h"
#include "mythryl-callable-cfun-hashtable.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-globals.h"


static Bool	repair_heap__local;					// This is TRUE, as long as it is cheaper
									// to repair the heap than to complete
									// the cleaning.

static Bool  finishing_cleaning__local;				// TRUE, when we are finishing a post-pickling cleaning.
static int   oldest_agegroup_being_cleaned__local;			// Oldest agegroup being cleaned.
static Val*  saved_top__local[ MAX_AGEGROUPS ][ MAX_PLAIN_SIBS ];	// Remember current endpoints of the to-space buffers.

static Heapfile_Cfun_Table*   cfun_table__local;				// Table of Mythryl-callable C functions in runtime which are referenced from heap.  (May also contain some assembly fns, refcells and exceptions.)
static Addresstable*   embedded_chunk_ref_table__local;			// Table of embedded chunk references.

/* typedef   struct repair   Repair; */					// in src/c/h/heap.h

struct repair {
    //
    Val* loc;								// Location to repair.
    Val	 val;								// Old value.
};

// Record a location in a given sib buffer for repair:
//
#define NOTE_REPAIR(sib, location, value)	{	\
	Sib* __sib = (sib);				\
	if (repair_heap__local) {				\
	    Repair	*__rp = __sib->repairlist - 1;	\
	    if ((Val *)__rp > __sib->next_tospace_word_to_allocate) {		\
		__rp->loc = (location);			\
		__rp->val = (value);			\
		__sib->repairlist = __rp;		\
	    }						\
	    else					\
		repair_heap__local = FALSE;			\
	}						\
    }


 static void                   repair_heap				(Task* task,  int max_age);
 static void                   wrap_up_cleaning				(Task* task,  int max_age);
 static void                   swap_tospace_with_fromspace		(Task* task,  int age);

 static Status                 sweep_tospace				(Task* task,  Sibid max_aid);
 static Val                    forward_chunk				(Task* task,  Val chunk, Sibid id);
 static Hugechunk*             forward_hugechunk			(Task* task,  Val* p,  Val chunk,  Sibid aid);

 static Embedded_Chunk_Info*   find_embedded_chunk			(Addresstable* table,  Punt addr,  Embedded_Chunk_Kind kind);			// Embedded_Chunk_Kind		def in   src/c/heapcleaner/datastructure-pickler.h
 static void                   record_addresses_of_extracted_literals	(Punt addr, void *_closure, void *_info);					// This is UNIMPLEMENTED!
 static void                   extract_literals_from_codechunks		(Punt addr, void *_closure, void *_info);

// The closure for record_addresses_of_extracted_literals:
//
struct assignlits_clos {
    //
    Val_Sized_Unt  id;		  			// The heap image chunk index for embedded literals.
    Val_Sized_Unt  offset;				// The offset of the next literal.
};

// The closure for extract_literals_from_codechunks:
//
struct extractlits_clos {
    //
    Writer*	   wr;
    Val_Sized_Unt  offset;				// The offset of the next literal; this is used to align reals.
};


// Check to see if we need to extend
// the number of flipped agegroups:
//
#define CHECK_AGEGROUP(task, g)	{			\
	int	__g = (g);				\
	if (__g > oldest_agegroup_being_cleaned__local)	\
	    swap_tospace_with_fromspace ((task), __g);			\
    }

// CHECK_WORD_FOR_EXTERNAL_REFERENCE:
//
// Check a Mythryl value for external references, etc.
//
#define CHECK_WORD_FOR_EXTERNAL_REFERENCE(task, book2sibid, p, maxAid, seen_error) {					\
	Val	__w = *(p);										\
	/*debug_say ("CheckWord @ %#x --> %#x: ", p, __w);*/						\
	if (IS_POINTER(__w)) {										\
	    Sibid	__aid = SIBID_FOR_POINTER(book2sibid, __w);					\
	    if (BOOK_IS_UNMAPPED(__aid)) {								\
	      /* An external reference */								\
	      /*debug_say ("external reference\n");*/							\
		if ((! finishing_cleaning__local) && (add_cfun_to_heapfile_cfun_table(cfun_table__local, __w) == HEAP_VOID))	\
		    (seen_error) = TRUE;								\
	    } else if (SIBID_KIND_IS_CODE(__aid))							\
		/*{debug_say ("hugechunk\n");*/								\
		forward_hugechunk(task, p, __w, __aid);						\
	/*}*/												\
	    else if (SIBID_IS_IN_FROMSPACE(__aid, maxAid))						\
		/*{debug_say ("regular chunk\n");*/							\
		*(p) = forward_chunk(task, __w, __aid);						\
	/*}*/												\
	}												\
	/*else debug_say ("unboxed \n");*/								\
    }



Pickler_Result   pickler__clean_heap   (
    //           ===================
    //
    Task* task,
    Val*  root_chunk,
    int   age		// Caller guarantees age == get_chunk_age( root_chunk )				// get_chunk_age			def in   src/c/heapcleaner/get-chunk-age.c
){
//  Heap* heap =  task->heap;

    Sibid* b2s =  book_to_sibid__global;									// Cache global in register for speed.

    Bool seen_error =  FALSE;

    Pickler_Result result;										// Pickler_Result				def in   src/c/heapcleaner/datastructure-pickler.h

    result.oldest_agegroup_included_in_pickle
	=
	oldest_agegroup_being_cleaned__local;

    result.heap_needs_repair	= repair_heap__local;

    // Allocate the cfun and embedded chunk tables:
    //
    cfun_table__local =  make_heapfile_cfun_table();							// make_heapfile_cfun_table		def in   src/c/heapcleaner/mythryl-callable-cfun-hashtable.c
    //
    embedded_chunk_ref_table__local
	=
	make_address_hashtable( /*ignore_bits:*/ LOG2_BYTES_PER_WORD, /*buckets:*/ 64 );		// make_address_hashtable		def in   src/c/heapcleaner/address-hashtable.c

    result.cfun_table           =  cfun_table__local;
    result.embedded_chunk_table =  embedded_chunk_ref_table__local;

    // Initialize, by flipping the agegroups
    // up to the one including root_chunk:
    //
    repair_heap__local = TRUE;
    finishing_cleaning__local = FALSE;
    oldest_agegroup_being_cleaned__local = 0;
    swap_tospace_with_fromspace( task, age );

    // Scan root_chunk:
    //
    CHECK_WORD_FOR_EXTERNAL_REFERENCE (task, b2s, root_chunk, MAX_SIBID, seen_error);
    if (seen_error) {
	result.error = TRUE;
	return result;
    }

    if (sweep_tospace (task, MAX_SIBID) == FAILURE) {
	result.error = TRUE;
	return result;
    }

    result.error = FALSE;

    return result;
}							// fun pickler__clean_heap.


// pickler__relocate_embedded_literals:
//
// Assign relocation addresses to the embedded literals that are going to be
// extracted.  The arguments to this are the pickler result (containing the
// embedded literal table), the ID of the heap image chunk that the string
// literals are to be stored in, and the starting offset in that chunk.
// This returns the address immediately following the last embedded literal.
//
// NOTE: This code will break if the size of the string space
// plus embedded literals exceeds 16Mb.			XXX BUGGO FIXME
///
Punt   pickler__relocate_embedded_literals   (
    // =========================================================
    //
    Pickler_Result*     result,
    int               id,
    Punt offset
){
    struct assignlits_clos closure;
    //
    closure.offset = offset;
    closure.id = id;
    //
    addresstable_apply( embedded_chunk_ref_table__local, &closure, record_addresses_of_extracted_literals );		// addresstable_apply		def in    src/c/heapcleaner/address-hashtable.c
    //
    return closure.offset;
}


void   pickler__pickle_embedded_literals   (Writer *wr)   {
    // ====================================
    //
    // Pickle the embedded literals.


    struct extractlits_clos  closure;

    closure.wr = wr;
    closure.offset = 0;

    addresstable_apply( embedded_chunk_ref_table__local, &closure, extract_literals_from_codechunks );
}


void   pickler__wrap_up   (Task* task,  Pickler_Result* result)   {
    // ================
    //
    // Finish up the pickle-datastructure operation.
    // This means either repairing the heap or
    // completing the cleaning.

    if (result->heap_needs_repair)	repair_heap (task, result->oldest_agegroup_included_in_pickle);
    else				wrap_up_cleaning   (task, result->oldest_agegroup_included_in_pickle);

    free_heapfile_cfun_table( cfun_table__local );

    free_address_table( embedded_chunk_ref_table__local, TRUE );
}								// fun pickler__wrap_up


static void   repair_heap   (Task* task,  int max_age)   {
    //        ===========
    //
    Heap*  heap =  task->heap;

    for (int i = 0;  i < max_age;  i++) {
        //
	Agegroup* age = heap->agegroup[i];

	#define REPAIR(index)	{						\
		Sib* __sib = age->sib[ index ];				\
		if (sib_is_active(__sib)) {						\
		    Repair	*__stop, *__rp;					\
		    __stop = (Repair*)(__sib->tospace_limit);				\
		    for (__rp = __sib->repairlist;  __rp < __stop;  __rp++) {	\
			Val	*__p = __rp->loc;				\
			if (index != RO_CONSCELL_SIB)				\
			    __p[-1] = FOLLOW_FORWARDING_POINTER(__p)[-1];			\
			__p[0] = __rp->val;					\
		    }								\
		}								\
	    }

	// Repair the sib buffers:
        //
	REPAIR( RO_POINTERS_SIB	);
	REPAIR( RO_CONSCELL_SIB	);
	REPAIR( NONPTR_DATA_SIB	);
	REPAIR( RW_POINTERS_SIB	);

	// Free the to-space chunk and reset the BOOK2SIBID marks:
        //
	for (int j = 0;  j < MAX_PLAIN_SIBS;  j++) {
	    //
	    Sib* sib =  age->sib[ j ];

	    if (sib_is_active(sib)) {							// sib_is_active	def in    src/c/h/heap.h

		// Unflip the spaces.
                // Note that free_agegroup
		// needs the from-space information.

		Val*	tmpBase = sib->tospace;
		Punt		tmpSizeB = sib->tospace_bytesize;
		Val*	tmpTop = sib->tospace_limit;
		sib->next_tospace_word_to_allocate	=
		sib->next_word_to_sweep_in_tospace = sib->fromspace_used_end;
		sib->tospace	= sib->fromspace;
		sib->fromspace	= tmpBase;
		sib->tospace_bytesize	= sib->fromspace_bytesize;
		sib->fromspace_bytesize	= tmpSizeB;
		sib->tospace_limit	= saved_top__local[i][j];
		sib->fromspace_used_end	= tmpTop;
	    }
	}



        // Free the to-space memory chunk:

	Quire* tmpChunk =  age->fromspace_ram_region;

	age->fromspace_ram_region =  age->tospace_ram_region;
	age->tospace_ram_region   =  tmpChunk;

	free_agegroup( heap, i );
    }
}								// fun repair_heap


static void   wrap_up_cleaning   (Task* task,  int max_age)   {
    //        ====================
    //
    // Complete the partial garbage collection.

    Heap*   heap       =  task->heap;
    Sibid*  b2s =  book_to_sibid__global;

    Bool dummy = FALSE;

    Sibid	maxAid;

    finishing_cleaning__local = TRUE;
    maxAid = MAKE_MAX_SIBID(max_age);

    // Allocate new coarse_inter_agegroup_pointers_map vectors for
    // the flipped agegroups:
    //
    for (int i = 0;  i < max_age;  i++) {
        //
	Agegroup* ag =  heap->agegroup[ i ];

	if (sib_is_active( ag->sib[ RW_POINTERS_SIB ] )) {							// sib_is_active	def in    src/c/h/heap.h
	    //
	    make_new_coarse_inter_agegroup_pointers_map_for_agegroup( ag );
	}
    }

    // Collect the roots

    #define CHECK_ROOT(p)	{					\
	    Val	*__p = (p);				\
	    CHECK_WORD_FOR_EXTERNAL_REFERENCE (task, b2s, __p, maxAid, dummy);	\
	}

    for (int i = 0;  i < c_roots_count__global;  i++) {
	//
	CHECK_ROOT(c_roots__global[i]);
    }

    CHECK_ROOT( &task->argument				);
    CHECK_ROOT( &task->fate				);
    CHECK_ROOT( &task->current_closure			);
    CHECK_ROOT( &task->link_register			);
    CHECK_ROOT( &task->program_counter			);
    CHECK_ROOT( &task->exception_fate			);
    CHECK_ROOT( &task->current_thread				);
    CHECK_ROOT( &task->callee_saved_registers[0]		);
    CHECK_ROOT( &task->callee_saved_registers[1]		);
    CHECK_ROOT( &task->callee_saved_registers[2]		);

    // Sweep the dirty pages of agegroups over max_age:
    //
    for (int i = max_age;  i < heap->active_agegroups;  i++) {
        //
	Agegroup* ag =  heap->agegroup[ i ];

	if (sib_is_active( ag->sib[ RW_POINTERS_SIB ] )) {							// sib_is_active	def in    src/c/h/heap.h
	    //
	    Coarse_Inter_Agegroup_Pointers_Map*	map = ag->coarse_inter_agegroup_pointers_map;

	    if (map != NULL) {
		//
		Val* maxSweep =  ag->sib[ RW_POINTERS_SIB ]->next_word_to_sweep_in_tospace;

		int  card;


		FOR_ALL_DIRTY_CARDS_IN_CARDMAP (map, max_age, card, {
		    //
		    Val*  p =  (map->base_address + (card*CARD_SIZE_IN_WORDS));
		    Val*  q =  p + CARD_SIZE_IN_WORDS;

		    int mark =  i+1;

		    if (q > maxSweep)   q = maxSweep;		// Don't sweep above the allocation high-water mark.

		    for (;  p < q;  p++) {

			Val	w = *p;

			if (IS_POINTER(w)) {

			    Sibid aid =  SIBID_FOR_POINTER(b2s, w);

			    int target_age;

			    if (SIBID_IS_IN_FROMSPACE(aid, maxAid)) {	// This is a from-space chunk.

				if (SIBID_KIND_IS_CODE(aid)) {

				    Hugechunk*	dp =  forward_hugechunk (task, p, w, aid);

				    target_age = dp->age;

				} else {

				    *p = w = forward_chunk(task, w, aid);

				    target_age =  GET_AGE_FROM_SIBID( SIBID_FOR_POINTER( b2s, w ));
				}

				if (mark > target_age) {
				    mark = target_age;
				}
			    }
			}
		    }

		    // Re-mark the card:
		    //
		    ASSERT( map->min_age[card] <= mark );

		    if (mark <= i)	 	map->min_age[ card ] =  mark;
		    else if (i == max_age)	map->min_age[ card ] =  CLEAN_CARD;
		});
	    }
	}
    }

    sweep_tospace( task, maxAid );

    // Scan the vector sibs of the
    // flipped agegroups, marking dirty pages:
    //
    for (int i = 1;  i < max_age;  i++) {
        //
	Agegroup* ag =  heap->agegroup[ i ];
        //
	Sib* sib =  ag->sib[ RW_POINTERS_SIB ];
        //
	if (sib_is_active(sib)) {							// sib_is_active	def in    src/c/h/heap.h
	    //
	    Coarse_Inter_Agegroup_Pointers_Map*   map
		=
		ag->coarse_inter_agegroup_pointers_map;

	    Val* stop;
	    Val  w;

	    int card = 0;
	    Val* p = sib->tospace;
	    while (p < sib->next_tospace_word_to_allocate) {

		int	mark = i+1;

		stop = (Val*) (( (Punt)p + CARD_BYTESIZE) & ~(CARD_BYTESIZE - 1));

		if (stop > sib->next_tospace_word_to_allocate) {
		    stop = sib->next_tospace_word_to_allocate;
                }

		while (p < stop) {

		    if (IS_POINTER(w = *p++)) {

			Sibid aid = SIBID_FOR_POINTER(b2s, w);

			int	 target_age;

			if (!SIBID_KIND_IS_CODE(aid)) {

			    target_age = GET_AGE_FROM_SIBID( aid );

			} else {

			    Hugechunk*  dp =   address_to_hugechunk( w );

			    target_age = dp->age;
			}

			if (target_age < mark) {

			    mark = target_age;

			    if (mark == 1) {
				p = stop;
				break;			// Nothing dirtier than 1st agegroup.
			    }
			}
		    }
		}

		if (mark <= i)   map->min_age[card] =  mark;
		else		 map->min_age[card] =  CLEAN_CARD;

		card++;
	    }
	}
    }

    // Reclaim space:
    //
    for (int i = 0;  i < max_age;  i++) {
	//
	free_agegroup( heap, i );

    }

    // Remember the top of to-space in
    // the cleaned agegroups:
    //
    for (int i = 0;  i < max_age;  i++) {
        //
	Agegroup* g =  heap->agegroup[ i ];
        //
	if (i == heap->active_agegroups-1) {
	    //
	    // The oldest agegroup has
            // only "young" chunks:
																	// sib_is_active	def in    src/c/h/heap.h
	    for (int j = 0;  j < MAX_PLAIN_SIBS;  j++) {
		//
		if (sib_is_active( g->sib[ j ] ))  g->sib[ j ]->end_of_fromspace_oldstuff =  g->sib[ j ]->tospace;
		else			           g->sib[ j ]->end_of_fromspace_oldstuff =  NULL;
	    }

	} else {

	    for (int j = 0;  j < MAX_PLAIN_SIBS;  j++) {
		//
		if (sib_is_active( g->sib[ j ] ))  g->sib[ j ]->end_of_fromspace_oldstuff =  g->sib[ j ]->next_tospace_word_to_allocate;
		else			           g->sib[ j ]->end_of_fromspace_oldstuff =  NULL;
	    }
	}
    }


    // Count the number of forwarded bytes:
    //
    for     (int g = 0;  g < max_age;   g++) {
	for (int a = 0;  a < MAX_PLAIN_SIBS;  a++) {
	    //
	    Sib* sib =  heap->agegroup[ g ]->sib[ a ];
	    //
	    if (sib_is_active(sib)) {
		//
		INCREASE_BIGCOUNTER(
		    //
		    &heap->total_bytes_copied_to_sib[ g ][ a ],
		    sib->next_tospace_word_to_allocate - sib->tospace
		);
	    }
	}
    }
}								// fun wrap_up_cleaning


static void   swap_tospace_with_fromspace   (Task* task, int gen) {
    //        ===============================
    // 
    // Flip additional agegroups from
    // oldest_agegroup_being_cleaned__local+1 .. gen.
    //
    // We allocate a to-space that is the
    // same size as the existing from-space.

    Heap* heap = task->heap;
    Punt  new_size;

    for (int age = oldest_agegroup_being_cleaned__local;  age < gen;  age++) {
        //
	Agegroup* g =  heap->agegroup[ age ];
        //
	for (int s = 0;  s < MAX_PLAIN_SIBS;  s++) {
	    //
	    Sib* sib = g->sib[ s ];
	    //
	    if (sib_is_active(sib)) {												// sib_is_active			def in    src/c/h/heap.h
		//
		ASSERT(
                    s == NONPTR_DATA_SIB
                    ||
                    sib->next_tospace_word_to_allocate == sib->next_word_to_sweep_in_tospace
                );

	        saved_top__local[age][s] = sib->tospace_limit;

		make_sib_tospace_into_fromspace( sib );										// make_sib_tospace_into_fromspace	def in    src/c/h/heap.h

		new_size = (Punt) sib->fromspace_used_end
                      - (Punt) sib->fromspace;

		if (age == 0)        new_size +=  agegroup0_buffer_size_in_bytes( task );					// Need to guarantee space for future minor collections.
		if (s == RO_CONSCELL_SIB)   new_size +=  2*WORD_BYTESIZE;								// We reserve (do not use) first slot in pairsib, so allocate extra space for it.

		sib->tospace_bytesize =  BOOKROUNDED_BYTESIZE( new_size );
	    }
	}
	g->fromspace_ram_region = g->tospace_ram_region;

	if (allocate_and_partition_an_agegroup(g) == FAILURE) {
	    //
	    die ("unable to allocate to-space for agegroup %d\n", age+1);
	}

        // Initialize the repair lists:
        //
	for (int ilk = 0;  ilk < MAX_PLAIN_SIBS;  ilk++) {
	    //
	    Sib* sib =  g->sib[ ilk ];

	    if (sib_is_active(sib))   sib->repairlist = (Repair*)(sib->tospace_limit);						// sib_is_active			def in    src/c/h/heap.h
	}
    }

    oldest_agegroup_being_cleaned__local = gen;
}										// fun swap_tospace_with_fromspace

static Status   sweep_tospace   (Task*  task,   Sibid  maxAid) {
    //          =============
    // 
    // Sweep the to-space sib buffers.
    // Because there are few references forward in time, 
    // we try to completely scavenge a younger agegroup
    // before moving on to the next oldest.

    Heap*	heap = task->heap;

    Bool	swept;
    Sibid*	b2s =  book_to_sibid__global;
    Bool	seen_error = FALSE;

    #define SWEEP_SIB_TOSPACE_BUFFER(ag, index)	{						\
	    Sib* __sib = (ag)->sib[ index ];					\
	    if (sib_is_active(__sib)) {							\
		Val    *__p, *__q;							\
		__p = __sib->next_word_to_sweep_in_tospace;						\
		if (__p < __sib->next_tospace_word_to_allocate) {						\
		    swept = TRUE;							\
		    do {								\
			for (__q = __sib->next_tospace_word_to_allocate;  __p < __q;  __p++) {			\
			    CHECK_WORD_FOR_EXTERNAL_REFERENCE(task, b2s, __p, maxAid, seen_error);	\
			}								\
		    } while (__q != __sib->next_tospace_word_to_allocate);					\
		    __sib->next_word_to_sweep_in_tospace = __q;						\
		}									\
	    }										\
	}

    do {
	swept = FALSE;

	for (int g = 0;  g < oldest_agegroup_being_cleaned__local;  g++) {
	    //
	    Agegroup*  ag =   heap->agegroup[ g ];

	    // Sweep the record and pair sib buffers:
            //
	    SWEEP_SIB_TOSPACE_BUFFER( ag, RO_POINTERS_SIB );
	    SWEEP_SIB_TOSPACE_BUFFER( ag, RO_CONSCELL_SIB );
	    SWEEP_SIB_TOSPACE_BUFFER( ag, RW_POINTERS_SIB );
	}
    } while (swept && !seen_error);

    return  (seen_error ? FAILURE : SUCCESS);
}								// fun sweep_tospace


static Val   forward_chunk   (Task* task,   Val v,  Sibid id) {
    //       =============
    // 
    // Forward a chunk.

    Heap* heap = task->heap;
    Val* chunk = PTR_CAST(Val*, v);

    int	 gen = GET_AGE_FROM_SIBID(id);

    Val* new_chunk;

    Val	          tagword  =  HEAP_NIL;		// Initialized only to quiet "possibly unused" gcc warning.
    Val_Sized_Unt len   =  0;			// Initialized only to quiet "possibly unused" gcc warning.
    Sib*          sib   =  NULL;		// Initialized only to quiet "possibly unused" gcc warning.

    if (! finishing_cleaning__local)   CHECK_AGEGROUP(task, gen);

    switch (GET_KIND_FROM_SIBID(id)) {
	//
    case RO_POINTERS_KIND:
        {
	    tagword = chunk[-1];

	    switch (GET_BTAG_FROM_TAGWORD(tagword)) {
		//
	    case RO_VECTOR_HEADER_BTAG:
	    case RW_VECTOR_HEADER_BTAG:
		len = 2;
		break;

	    case FORWARDED_CHUNK_BTAG:
		return PTR_CAST( Val, FOLLOW_FORWARDING_POINTER(chunk));	        // This chunk has already been forwarded.

	    default:
		len = GET_LENGTH_IN_WORDS_FROM_TAGWORD(tagword);
	    }
	    sib =  heap->agegroup[ gen-1 ]->sib[ RO_POINTERS_SIB ];
	}
        break;

    case RO_CONSCELL_KIND:
	{
	    Val	w;

	    w = chunk[0];
	    if (IS_TAGWORD(w))  return PTR_CAST( Val, FOLLOW_PAIRSPACE_FORWARDING_POINTER(w, chunk));

	    // Forward the pair:
	    //
	    sib =  heap->agegroup[ gen-1 ]->sib[ RO_CONSCELL_SIB ];

	    new_chunk =  sib->next_tospace_word_to_allocate;

	    sib->next_tospace_word_to_allocate += 2;

	    new_chunk[0] = w;
	    new_chunk[1] = chunk[1];

	    // Set up the forward pointer in the old pair:
	    //
	    NOTE_REPAIR( sib, chunk, w );
	    //
	    chunk[0] =  MAKE_PAIRSPACE_FORWARDING_POINTER( new_chunk );
	    //
	    return PTR_CAST( Val,  new_chunk );
	}
	break;

    case NONPTR_DATA_KIND:
	{
	    sib =  heap->agegroup[ gen-1 ]->sib[ NONPTR_DATA_SIB ];
	    tagword = chunk[-1];

	    switch (GET_BTAG_FROM_TAGWORD(tagword)) {
		//
	    case FORWARDED_CHUNK_BTAG:
		return PTR_CAST( Val, FOLLOW_FORWARDING_POINTER(chunk));

	    case FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG:
		len = GET_LENGTH_IN_WORDS_FROM_TAGWORD(tagword);
		break;

	    case EIGHT_BYTE_ALIGNED_NONPOINTER_DATA_BTAG:
		len = GET_LENGTH_IN_WORDS_FROM_TAGWORD(tagword);
		#ifdef ALIGN_FLOAT64S
		#  ifdef CHECK_HEAP
			    if (((Punt) sib->next_tospace_word_to_allocate & WORD_BYTESIZE) == 0) {
				//
				*sib->next_tospace_word_to_allocate = (Val)0;
				sib->next_tospace_word_to_allocate++;
			    }
		#  else
			    sib->next_tospace_word_to_allocate = (Val *)(((Punt) sib->next_tospace_word_to_allocate) | WORD_BYTESIZE);
		#  endif
		#endif
		break;

	    default:
		die ("bad string tag %d, chunk = %#x, tagword = %#x",
		    GET_BTAG_FROM_TAGWORD(tagword), chunk, tagword);
	    }
	}
	break;

    case RW_POINTERS_KIND:
	{
	    tagword = chunk[-1];

	    switch (GET_BTAG_FROM_TAGWORD(tagword)) {
		//
	    case FORWARDED_CHUNK_BTAG:
		return PTR_CAST( Val, FOLLOW_FORWARDING_POINTER(chunk));		// This chunk has already been forwarded.

	    case RW_VECTOR_DATA_BTAG:
		len = GET_LENGTH_IN_WORDS_FROM_TAGWORD(tagword);
		break;

	    case WEAK_POINTER_OR_SUSPENSION_BTAG:
	        // We are conservative here -- we
		// never nullify special chunks:
		len = 1;
		break;

	    default:
		die ( "Fatal error: bad rw_vector b-tag %#x, chunk = %#x, tagword of chunk = %#x (= chunk[-1]) tag should be one of  %#x %#x %#x -- src/c/heapcleaner/datastructure-pickler-cleaner.c",
		      GET_BTAG_FROM_TAGWORD( tagword ),
		      chunk,
		      tagword,
		      FORWARDED_CHUNK_BTAG,
		      RW_VECTOR_DATA_BTAG,
		      WEAK_POINTER_OR_SUSPENSION_BTAG
		    );

                exit(1);													// Cannot execute -- just to quiet gcc -Wall.
	    }

	    sib =  heap->agegroup[ gen-1 ]->sib[ RW_POINTERS_SIB ];
	}
	break;

    case CODE_KIND:
    default:
	die("forward_chunk: unknown chunk ilk %d @ %#x", GET_KIND_FROM_SIBID(id), chunk);
    }

    // Allocate and initialize a to-space copy of the chunk:
    //
    new_chunk =  sib->next_tospace_word_to_allocate;

    sib->next_tospace_word_to_allocate
	+=
	len + 1;

    *new_chunk++ = tagword;

    COPYLOOP(chunk, new_chunk, len);

    // Set up the forward pointer
    // and return the new chunk:
    //
    NOTE_REPAIR( sib, chunk, *chunk );

    chunk[-1] = FORWARDED_CHUNK_TAGWORD;

    chunk[0] =  (Val)(Punt)  new_chunk;

    return PTR_CAST( Val, new_chunk);
}									// fun forward_chunk


static Hugechunk*   forward_hugechunk   (
    //              =================
    //
    Task*  task,
    Val*   p,
    Val	   chunk,
    Sibid   sibid
){
    // 
    // Forward a hugechunk.
    // 'id' is the BOOK2SIBID entry for chunk.
    // Return the hugechunk tagword.
    // NOTE: We do not ``promote'' hugechunk here
    // because they are not reclaimed when completing
    // the cleaning.

//  Heap*                 heap = task->heap;

    Hugechunk_Region*     region;
    Hugechunk*            dp;
    Embedded_Chunk_Info*  code_info;

    {	int  i;
	for (i = GET_BOOK_CONTAINING_POINTEE(chunk);  !SIBID_ID_IS_BIGCHUNK_RECORD(sibid);  sibid = book_to_sibid__global[ --i ]);
	//
	region = (Hugechunk_Region*) ADDRESS_OF_BOOK( i );
    }

    dp     = get_hugechunk_holding_pointee(region, chunk);

    if (! finishing_cleaning__local) {
        //
	CHECK_AGEGROUP(task, dp->age);

	code_info = find_embedded_chunk( embedded_chunk_ref_table__local, dp->chunk, UNUSED_CODE );

	code_info->kind = USED_CODE;
    }

    return dp;
}									// fun forward_hugechunk


static Embedded_Chunk_Info*   find_embedded_chunk   (
    //                        ===================
    //
    Addresstable*         table,
    Punt                  addr,
    Embedded_Chunk_Kind   kind
) {
    Embedded_Chunk_Info* p  =  FIND_EMBEDDED_CHUNK( table, addr );	// FIND_EMBEDDED_CHUNK		def in    src/c/heapcleaner/datastructure-pickler.h

    if (p == NULL) {
	p		= MALLOC_CHUNK(Embedded_Chunk_Info);
	p->kind		= kind;
	p->containing_codechunk	= NULL;
	addresstable_insert(table, addr, p);
    }

    ASSERT( kind == p->kind );

    return p;
}									 // fun find_embedded_chunk

static void   record_addresses_of_extracted_literals   (
    //        ======================================
    //
    Punt addr,
    void* _closure,
    void* _info
){
    // Calculate the locations of the extracted literal
    // strings in the datastructure pickle and record their addresses.
    //
    // This function is passed as an argument to addresstable_apply;
    // its second argument is its "closure," and its third
    // argument is the embedded chunk info.

    #ifdef XXX
	struct assignlits_clos *closure = (struct assignlits_clos*) _closure;

	Embedded_Chunk_Info*  info = (Embedded_Chunk_Info*) _info;

	int chunk_bytesize;

	switch (info->kind) {
	    //
	case UNUSED_CODE:
	case USED_CODE:
	    info->relocated_address = (Val)0;
	    return;

	case EMBEDDED_STRING:
	    {
		int		nChars = CHUNK_LENGTH(PTR_CAST( Val, addr));
		int		nWords = BYTES_TO_WORDS(nChars);

		if ((nChars != 0) && ((nChars & 0x3) == 0))   nWords++;

		chunk_bytesize = nWords * WORD_BYTESIZE;
	    }
	    break;

	case EMBEDDED_FLOAT64:
	    chunk_bytesize = CHUNK_LENGTH(PTR_CAST( Val, addr)) * FLOAT64_BYTESIZE;
	    #ifdef ALIGN_FLOAT64S
		    closure->offset |= WORD_BYTESIZE;
	    #endif
	    break;
	}

	if (info->containing_codechunk->kind == USED_CODE) {	    // The containing code chunk is also being exported.
	    info->relocated_address = (Val)0;
	    return;
	}

	if (chunk_bytesize == 0) {
	    //
	    info->relocated_address
		=
		add_cfun_to_heapfile_cfun_table (
                    cfun_table__local,
		    (info->kind == EMBEDDED_STRING) ? ZERO_LENGTH_STRING__GLOBAL : LIB7_realarray0
                );
	} else {
	    // Assign a relocation address to the chunk
            // and bump the offset counter:
            //
	    closure->offset += WORD_BYTESIZE;				// Space for the tagword.
	    info->relocated_address = HIO_TAG_PTR(closure->id, closure->offset);
	    closure->offset += chunk_bytesize;
	}
    #else
        die ("record_addresses_of_extracted_literals");
    #endif
}										// fun record_addresses_of_extracted_literals



static void   extract_literals_from_codechunks   (
    //        ================================
    //
    Punt  addr,
    void*              _closure,						// Matthias calls this a closure; it's just our state.
    void*              _info
) {
    // Extract the embedded literals that are
    // in otherwise unreferenced code blocks.
    //
    // This function is passed as an argument to addresstable_apply.
    // Its second argument is its "closure";
    // its third argument is the embedded chunk info.

    struct extractlits_clos*  closure
	=
	(struct extractlits_clos*) _closure;

    Embedded_Chunk_Info*  info
	=
	(Embedded_Chunk_Info*) _info;

    int  chunk_bytesize = 0;							// Initialization is redundant; done just to suppress compiler "may be used uninitialized" warning.

    if (info->relocated_address == (Val)0)   return;

    switch (info->kind) {
	//
    case EMBEDDED_STRING:
        {
	    int char_count =  CHUNK_LENGTH(PTR_CAST( Val, addr));
	    int word_count =  BYTES_TO_WORDS(char_count);

	    if (char_count != 0
            && (char_count & 0x3) == 0
            ){
	        word_count++;							// Ick! XXX BUGGO FIXME
	    }

	    chunk_bytesize = word_count * WORD_BYTESIZE;
	}
	break;

    case EMBEDDED_FLOAT64:
	//
	chunk_bytesize = CHUNK_LENGTH(PTR_CAST( Val, addr)) * FLOAT64_BYTESIZE;

	#ifdef ALIGN_FLOAT64S
	    //
	    if ((closure->offset & (FLOAT64_BYTESIZE-1)) == 0) {
		//
		// The tagword would be 8-byte aligned,
		// which means that the Float64 would not be,
		// so add some padding:
		//
		WR_PUT(closure->wr, 0);
		closure->offset += 4;
	    }
	#endif
	break;

    case UNUSED_CODE:
    case USED_CODE:
        die ("record_addresses_of_extracted_literals: UNUSED_CODE and USED_CODE cases unimplemented");	// Added 2010-11-22 CrT to quiet a compiler warning.
    }

    if (chunk_bytesize != 0) {
        //
        // Write the chunk into the pickle buffer,
        // including the tagword:

	WR_WRITE( closure->wr, (void *)(addr - WORD_BYTESIZE), chunk_bytesize + WORD_BYTESIZE);
	//
	closure->offset += (chunk_bytesize + WORD_BYTESIZE);
    }
}									// fun extract_literals_from_codechunks


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

