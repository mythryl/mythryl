// make-strings-and-vectors-etc.c
//
// Support functions which mostly create various kinds of
// strings and vectors on the Mythryl heap.
//
// Multicore (hostthread) note: when invoking the heapcleaner
// we add the requested size to requested_extra_free_bytes,
// so that multiple processors can request space at the same time.


/*
###             Ballade to an Artificial Satellite
###
###		One inland summer I walked through rye
###		A wind at my heels that smelled of grain
###		And harried white clouds through a whistling sky
###		Where the great sun stalked and shook his mane
###		And roared so brightly across the plain
###		It gleamed and shimmered like alien sands
###		Ten years old I saw down a lane
###		The thunderous light on wonderstrands.
###
###		In ages before the world ran dry
###		What might the mapless not contain?
###		Atlantis gleamed like a dream to die
###		Avalon lay under faerie reign
###		Cibola guarded a golden plain
###		Tir nan Og was fairlocked Fand's
###		Sober men saw from a gull's road wain
###		The thunderous light on wonderstrands
###
###		Such clanging countries in cloud lands lie
###		But men grew weary and men grew sane
###		And they grew grown and so did I
###		And knew Tartessus was only Spain
###		No galleons call at Taprobane --
###		Ceylon with English -- no queenly hand
###		Wears gold from Punt nor sees the Dane
###		The thunderous light on wonderstrands.
###
###		Ahoy, Prince Andros, horizon's bane!
###		They always wait, the elven lands
###		An evening planet sheds again
###		The thunderous light on wonderstrands.
###
###                       -- Poul Anderson
*/


#include "../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "runtime-base.h"
#include "heap.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-configuration.h"


// A cleaning-needed check.
// On multicore systems this needs to be
// a loop because other hostthreads may
// steal the memory before the checking
// hostthread can use it.
																// sib_is_active		def in    src/c/h/heap.h
																// sib_freespace_in_bytes	def in    src/c/h/heap.h


#define WHILE_INSUFFICIENT_FREESPACE_IN_SIB(ap, bytes)  while ((! sib_is_active(ap)) || (sib_freespace_in_bytes(ap) <= (bytes)))

#define COUNT_ALLOC(task, nbytes)	{	\
	Heap		*__h = task->heap;	\
	INCREASE_BIGCOUNTER(&(__h->total_bytes_allocated), (nbytes));	\
    }

//
Val   make_ascii_string_from_c_string__may_heapclean   (Task* task,  const char* v,  Roots* extra_roots)   {
    //==============================================
    // 
    // Allocate a Mythryl string using a C string as an initializer.
    // We assume that the string is small and can be allocated in
    // the agegroup0 buffer.        XXX BUGGO FIXME

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);


    int len =   v == NULL  ?  0
                           :  strlen(v);

    if (len == 0) {
        //
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
	return ZERO_LENGTH_STRING__GLOBAL;
        //
    } else {
        //
	int	    n = BYTES_TO_WORDS(len+1);				// '+1' to count terminal '\0' also.

	Val result = allocate_nonempty_wordslots_vector__may_heapclean( task, n, extra_roots );

	// Zero the last word to allow fast (word) string comparisons,
	// and to guarantee 0 termination:
	//
	PTR_CAST( Vunt*, result) [n-1] = 0;
	strcpy (PTR_CAST(char*, result), v);

	result =  make_vector_header(task, STRING_TAGWORD, result, len);
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
	return result;
    }
}

//
Val   make_ascii_strings_from_vector_of_c_strings__may_heapclean   (Task *task, char** strs,  Roots* extra_roots)   {
    //==========================================================
    // 
    // Given a NULL terminated rw_vector of char*, build a list of Mythryl strings.
    //
    // NOTE: we should do something about possible GC!!! XXX BUGGO FIXME**/


									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int	 i;
    for (i = 0;  strs[i] != NULL;  i++);

    Val p = LIST_NIL;								Roots roots1 = { &p, extra_roots };

    while (i-- > 0) {
	//
	Val s =  make_ascii_string_from_c_string__may_heapclean( task, strs[i], &roots1 );
	//
	p = LIST_CONS(task, s, p);
    }
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);

    return p;
}

//
Val   allocate_headerless_ascii_string__may_heapclean   (Task* task,  int len,  Roots* extra_roots)   {
    //========================================================
    // 
    // Allocate an uninitialized Mythryl string of length > 0.
    // This string is guaranteed to be padded to word size with 0 bytes,
    // and to be 0 terminated.

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int		nwords = BYTES_TO_WORDS( len +1 );					// '+1' is probably to allow for a terminal nul ('\0'), mostly to promote interoperability with C.

    ASSERT(len > 0);

    Val result = allocate_nonempty_wordslots_vector__may_heapclean( task, nwords, extra_roots );

    // Zero the last word to allow fast (word) string comparisons,
    // and to guarantee 0 termination:
    //
    PTR_CAST(Vunt*, result)[nwords-1] = 0;
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return  result;
}

//
Val   allocate_nonempty_ascii_string__may_heapclean   (Task* task,  int len,  Roots* extra_roots)   {
    //=============================================
    // 
    // Allocate an uninitialized Mythryl string of length > 0.
    // This string is guaranteed to be padded to word size with 0 bytes,
    // and to be 0 terminated.
									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Val result = allocate_headerless_ascii_string__may_heapclean( task, len, extra_roots );

    result = make_vector_header(task,  STRING_TAGWORD, result, len);
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

//
Val   allocate_nonempty_wordslots_vector__may_heapclean   (Task* task,  int nwords, Roots* roots)   {		// The "__may_heapclean" suffix is a warning to caller that the heapcleaner might be invoked in this fn,
    //=================================================								// potentially moving everything on the heap to a new address.
    // 
    // Allocate an uninitialized vector of word-sized slots.


										ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Val	tagword = MAKE_TAGWORD(nwords, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG);
    Val	result;
    Vunt  bytesize;

    ASSERT(nwords > 0);

    if (nwords <= MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS) {
	//
	set_slot_in_nascent_heapchunk (task, 0, tagword);
	result = commit_nascent_heapchunk (task, nwords);

    } else {

        // By policy we don't create vectors this large in agegroup0,
	// so we will instead create it in agegroup1, which (confusingly)
	// is addressed as task->heap->agegroup[0]:

	Sib* sib =   task->heap->agegroup[ 0 ]->sib[ NONPTR_DATA_SIB ];

	bytesize = WORD_BYTESIZE*(nwords + 1);

	pthread_mutex_lock( &pth__mutex );								// Agegroup1 is shared with other hostthreads, so grab the lock on it.
	    //
	    WHILE_INSUFFICIENT_FREESPACE_IN_SIB(sib, bytesize+task->heap_allocation_buffer_bytesize) {

	        // We need to heapclean agegroup1 to
                // clear out sufficient space in the
                // agegroup sib for our vector:
                //
		sib->requested_extra_free_bytes += bytesize;
                //
		pthread_mutex_unlock( &pth__mutex );
		    //
		    call_heapcleaner_with_extra_roots( task, 1, roots );
		    //
		pthread_mutex_lock( &pth__mutex );
                //
		sib->requested_extra_free_bytes = 0;
	    }

	    /////////////////////////////////////////////////////////	
	    // We now have enough sib space to create our vector.
	    // We cannot release the lock yet, because some other
	    // hostthread might steal the freespace out from under us.
	    /////////////////////////////////////////////////////////	
	
	    *(sib->tospace.first_free++) = tagword;							// Lay down the vector tagword.
	    result = PTR_CAST( Val, sib->tospace.first_free);						// Save pointer to start of vector proper -- our return value.
	    sib->tospace.first_free += nwords;								// Allocate sibspace for tagword plus vector.	
	    //
	pthread_mutex_unlock( &pth__mutex );								// NOW we can safely release the heap lock.

	COUNT_ALLOC(task, bytesize);
    }
										EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}
//
void   shrink_fresh_wordslots_vector   (Task* task,  Val v,  int new_length_in_words)   {
    // =============================
    // 
    // Shrink a freshly allocated vector with word-size slots.
    // This is used by the input routines that must pessimistically
    // pre-allocate space for more input than actually gets read:
    //     src/c/lib/socket/recvfrom.c
    //     src/c/lib/posix-io/read.c

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int old_length_in_words = CHUNK_LENGTH(v);

    if (new_length_in_words == old_length_in_words)   return;

    ASSERT( new_length_in_words > 0  &&  new_length_in_words < old_length_in_words );

    if (old_length_in_words > MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS) {
        //
	Sib*  sib = task->heap->agegroup[ 0 ]->sib[ NONPTR_DATA_SIB ];

	ASSERT(sib->tospace.first_free - old_length_in_words == PTR_CAST(Val*, v)); 

	sib->tospace.first_free -= (old_length_in_words - new_length_in_words);

    } else {

	ASSERT(task->heap_allocation_pointer - old_length_in_words == PTR_CAST(Val*, v)); 
	task->heap_allocation_pointer -= (old_length_in_words - new_length_in_words);
    }

    PTR_CAST(Val*, v)[-1] = MAKE_TAGWORD(new_length_in_words, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG);

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
}



// Allocate an uninitialized chunk of raw64 data.
// 
Val   allocate_biwordslots_vector__may_heapclean   (Task* task,  int nelems,  Roots* extra_roots)   {
    //==========================================

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int	nwords  =  DOUBLES_TO_WORDS(nelems);
    Val	tagword =  MAKE_TAGWORD(nwords, EIGHT_BYTE_ALIGNED_NONPOINTER_DATA_BTAG);
    Val	result;
    Vunt  bytesize;

    if (nwords <= MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS) {
	//
	#ifdef ALIGN_FLOAT64S
	    // Force FLOAT64_BYTESIZE alignment:
	    //
	    task->heap_allocation_pointer = (Val *)((Vunt)(task->heap_allocation_pointer) | WORD_BYTESIZE);
	#endif

	set_slot_in_nascent_heapchunk (task, 0, tagword);
	result = commit_nascent_heapchunk (task, nwords);

    } else {

	Sib* ap =   task->heap->agegroup[ 0 ]->sib[ NONPTR_DATA_SIB ];

	bytesize =  WORD_BYTESIZE*(nwords + 2);			// NOTE: we use nwords+2 to allow for the alignment padding.	// 64-bit issue.

	pthread_mutex_lock( &pth__mutex );
	    //
	    WHILE_INSUFFICIENT_FREESPACE_IN_SIB(ap, bytesize+task->heap_allocation_buffer_bytesize) {
		//
	        // We need to do a garbage collection:

		ap->requested_extra_free_bytes += bytesize;
		//
		pthread_mutex_unlock( &pth__mutex );
		    //
		    call_heapcleaner_with_extra_roots( task, 1, extra_roots );
		    //
		pthread_mutex_lock( &pth__mutex );
		//
		ap->requested_extra_free_bytes = 0;
	    }

	    #ifdef ALIGN_FLOAT64S
		//
		// Force FLOAT64_BYTESIZE alignment (tagword is off by one word)

	        #ifdef CHECK_HEAP
		    if (((Vunt)ap->tospace.first_free & WORD_BYTESIZE) == 0) {
			*(ap->tospace.first_free) = (Val)0;
			++ap->tospace.first_free;
		    }
	        #else
		    ap->tospace.first_free = (Val *)(((Vunt)ap->tospace.first_free) | WORD_BYTESIZE);
	        #endif
	    #endif

	    *ap->tospace.first_free ++
		=
		tagword;

	    result = PTR_CAST( Val, ap->tospace.first_free );

	    ap->tospace.first_free += nwords;

	pthread_mutex_unlock( &pth__mutex );

	COUNT_ALLOC(task, bytesize-WORD_BYTESIZE);
    }
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

//
Val   allocate_nonempty_code_chunk   (Task* task,  int len)   {
    //============================
    //
    // Allocate an uninitialized Mythryl code chunk.
    // Assume that len > 1.

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Heap*  heap = task->heap;

    int    allocGen = (heap->active_agegroups < CODECHUNK_ALLOCATION_AGEGROUP)
			? heap->active_agegroups
			: CODECHUNK_ALLOCATION_AGEGROUP;

    Agegroup* age =   heap->agegroup[ allocGen-1 ];

    Hugechunk* dp;

    pthread_mutex_lock( &pth__mutex );
	//
	dp = allocate_hugechunk (heap, allocGen, len);			// allocate_hugechunk		is from   src/c/heapcleaner/hugechunk.c
	ASSERT(dp->gen == allocGen);
	dp->next = age->hugechunks[ CODE__HUGE_SIB ];
	age->hugechunks[ CODE__HUGE_SIB ] = dp;
	dp->huge_ilk = CODE__HUGE_SIB;
	COUNT_ALLOC(task, len);
	//
    pthread_mutex_unlock( &pth__mutex );
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return PTR_CAST( Val, dp->chunk);
}

//
Val   allocate_nonempty_vector_of_one_byte_unts__may_heapclean   (Task* task,  int len,  Roots* extra_roots)   {
    //========================================================
    // 
    // Allocate an uninitialized Mythryl heap bytearray.  Assume that len > 0.

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int		nwords = BYTES_TO_WORDS(len);

    Val	result =  allocate_nonempty_wordslots_vector__may_heapclean( task, nwords, extra_roots );

    // Zero the last word to allow fast (word)
    // string comparisons, and to guarantee 0
    // termination:
    //
    PTR_CAST(Vunt*, result)[nwords-1] = 0;

    result =  make_vector_header(task,  UNT8_RW_VECTOR_TAGWORD, result, len);
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

//
Val   allocate_nonempty_vector_of_eight_byte_floats__may_heapclean   (Task* task,  int len,  Roots* extra_roots)   {
    //============================================================
    // 
    // Allocate an uninitialized Mythryl Float64 vector.  Assume that len > 0.
    //
    // Currently called (only) from REQUEST_ALLOCATE_VECTOR_OF_EIGHT_BYTE_FLOATS case in
    //
    //	   src/c/main/run-mythryl-code-and-runtime-eventloop.c

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Val result =  allocate_biwordslots_vector__may_heapclean( task, len, extra_roots );		// 64-bit issue.

    result = make_vector_header( task,  FLOAT64_RW_VECTOR_TAGWORD, result, len );
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

//
Val   allocate_headerless_rw_vector__may_heapclean   (Task* task,  int len,  Bool has_pointers, Roots* extra_roots)   {
    //=====================================================
    // 
    // Allocate a Mythryl rw_vector using init_val
    // as the initial value for vector slots.
    // Assume that len > 0.
    //
    // 'has_pointers' should be TRUE if the vector
    // might contain pointers (as opposed to Int31
    // immediate values.  (It is always safe to set
    // this to TRUE -- do so when in doubt.)

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Val	result;

    Val	tagword = MAKE_TAGWORD(len, RW_VECTOR_DATA_BTAG);


    Vunt	bytesize;

    if (len <= MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS) {
        //
	set_slot_in_nascent_heapchunk (task, 0, tagword);
	result = commit_nascent_heapchunk (task, len);
        //
    } else {
        //
	Sib*	ap = task->heap->agegroup[ 0 ]->sib[ RW_POINTERS_SIB ];

	// We have an issue here in that we cannot blithely
        // create pointers from agegroup1 to agegroup0 without
	// fouling up the heapcleaner ("garbage collector").
	//
	// We have to worry about this here because we are
	// creating this vector in agegroup1, but the pointer(s)
	// to be placed in it may point into agegroup0.
	//
	// We could do bookkeeping to track these (potential)
	// inter-agegroup pointers.  Here we instead adopt the
	// simpler solution of simply emptying agegroup0, thus
	// eliminating the problem. :-)
	//
	int gc_level = (has_pointers ? 0 : -1);				// gc_level == -1 means "Do not do a garbage collection."
									// gc_level ==  0 means "Empty (only) agegroup0".
									// gc_level ==  1 means "Empty agegroup0 and heapclean agegroup1 too."
	bytesize = WORD_BYTESIZE*(len + 1);

	pthread_mutex_lock( &pth__mutex );

	    clean_check: ;						// Hostthread support jumps to here to recheck for heapcleaning.

	    // If the agegroup1 sib we want to allocate in
	    // has not been created or does not have enough
	    // free space, we'll have to call the heapcleaner
	    // to establish it:
	    //	
	    if (! sib_is_active(ap)									// sib_is_active		def in    src/c/h/heap.h
		||
	        sib_freespace_in_bytes(ap) <= bytesize							// sib_freespace_in_bytes	def in    src/c/h/heap.h
                                              +
                                              task->heap_allocation_buffer_bytesize
            ){
		gc_level = 1;
	    }

	    if (gc_level >= 0) {
		//
	        // Clean heap:
                //
		ap->requested_extra_free_bytes += bytesize;
		pthread_mutex_unlock( &pth__mutex );
		    //
		    call_heapcleaner_with_extra_roots (task, gc_level, extra_roots );
		    //
		pthread_mutex_lock( &pth__mutex );
		ap->requested_extra_free_bytes = 0;

		// Check again to insure that we have sufficient space:					// Hostthread support.
		//
		gc_level = -1;
		goto clean_check;
	    }

	    ASSERT(ap->tospace.first_free == ap->tospace.swept_end);
	    *(ap->tospace.first_free++) = tagword;

	    result = PTR_CAST( Val, ap->tospace.first_free);

	    ap->tospace.first_free += len;
	    ap->tospace.swept_end = ap->tospace.first_free;
	    //
	pthread_mutex_unlock( &pth__mutex );

	COUNT_ALLOC(task, bytesize);

    }
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);

    return  result;
}											// fun make_nonempty_rw_vector__may_heapclean

//
Val   make_nonempty_rw_vector__may_heapclean   (Task* task,  int len,  Val init_val,  Roots* extra_roots)   {
    //======================================
    // 
    // Allocate a Mythryl rw_vector using init_val
    // as the initial value for vector slots.
    // Assume that len > 0.

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Roots roots1 = { &init_val, extra_roots };

    Val	result = allocate_headerless_rw_vector__may_heapclean(task, len, IS_POINTER(init_val), &roots1 );

    Val* p = PTR_CAST(Val*, result);
    //
    for (int i = 0;  i < len; i++) {
	//
	*p++ = init_val;
    }

    result =  make_vector_header(task,  TYPEAGNOSTIC_RW_VECTOR_TAGWORD, result, len);
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}											// fun make_nonempty_rw_vector__may_heapclean

//
Val   allocate_headerless_ro_pointers_chunk__may_heapclean   (Task* task,  int len,  int btag,  Roots* extra_roots)   {
    //====================================================
    // 
    // Allocate a Mythryl vector.
    // Assume that len > 0.
    //
    // 'tag' will be RO_VECTOR_DATA_BTAG or PAIRS_AND_RECORDS_BTAG.

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Val	tagword = MAKE_TAGWORD(len, btag);									// btag will be RO_VECTOR_DATA_BTAG or PAIRS_AND_RECORDS_BTAG.
    Val	result;

    if (len <= MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS) {
	//
	set_slot_in_nascent_heapchunk (task, 0, tagword);
	result = commit_nascent_heapchunk (task, len);
	//
    } else {
	//
	// Since we want to avoid pointers from the
        // agegroup 1 record space into the agegroup0 space,
	// we need to do a cleaning (while preserving our
	// initializer list).

	Sib* 	ap = task->heap->agegroup[ 0 ]->sib[ RO_POINTERS_SIB ];

	int	clean_level = 0;										// Heapclean agegroup0 only.

	Vunt  bytesize
	    =
	    WORD_BYTESIZE * (len+1);										// '+1' for tagword.

	pthread_mutex_lock( &pth__mutex );
	    //
	    if (! sib_is_active(ap)										// sib_is_active		def in    src/c/h/heap.h
		||
	        sib_freespace_in_bytes(ap) <= bytesize								// sib_freespace_in_bytes	def in    src/c/h/heap.h
                                              +
                                              task->heap_allocation_buffer_bytesize
	    ){
		clean_level = 1;										// Heapclean agegroup0 and agegroup1 too.
	    }

	    clean_check: ;											// Hostthread support jumps to here to redo the garbage collection.

	    ap->requested_extra_free_bytes += bytesize;
	    pthread_mutex_unlock( &pth__mutex );
		//
		call_heapcleaner_with_extra_roots (task, clean_level, extra_roots );
		//
	    pthread_mutex_lock( &pth__mutex );

	    ap->requested_extra_free_bytes = 0;

	    {   // Check again to ensure that we have sufficient space:						// Hostthread support.
		//
		if (sib_freespace_in_bytes(ap) <= bytesize + task->heap_allocation_buffer_bytesize)   goto clean_check;
	    }

	    ASSERT(ap->tospace.first_free == ap->tospace.swept_end);
	    *(ap->tospace.first_free++) = tagword;

	    result = PTR_CAST( Val,  ap->tospace.first_free );

	    ap->tospace.first_free += len;
	    ap->tospace.swept_end = ap->tospace.first_free;
	    //
	pthread_mutex_unlock( &pth__mutex );

	COUNT_ALLOC(task, bytesize);
    }
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return  result;
}													 // fun allocate_headerless_ro_pointers_chunk__may_heapclean

//
Val   make_nonempty_ro_vector__may_heapclean   (Task* task,  int len,  Val initializers,  Roots* extra_roots)   {
    //======================================
    // 
    // Allocate a Mythryl vector, using the
    // list initializers as an initializer.
    // Assume that len > 0.
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Roots roots1 = { &initializers, extra_roots };

    Val	result
	=
	allocate_headerless_ro_pointers_chunk__may_heapclean
	  ( task,
	    len,
	    RO_VECTOR_DATA_BTAG,
	    &roots1
	  );

    for (
        Val* p = PTR_CAST(Val*, result);
	initializers != LIST_NIL;
	initializers  = LIST_TAIL( initializers )
    ){
	*p++ = LIST_HEAD( initializers );
    }

    result =  make_vector_header( task,  TYPEAGNOSTIC_RO_VECTOR_TAGWORD, result, len );

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}													 // fun make_nonempty_ro_vector__may_heapclean

//
Val   make_system_constant__may_heapclean   (Task* task,  Sysconsts* table,  int id,  Roots* extra_roots)   {
    //===================================
    // 
    // Find the system constant with the given id
    // in table, and allocate a pair to represent it.
    //
    // If the constant is not present then
    // return the pair (~1, "<UNKNOWN>").

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    for (int i = 0;  i < table->constants_count;  i++) {
	//
	if (table->constants_vector[i].id == id) {
	    //
	    Val name =  make_ascii_string_from_c_string__may_heapclean (task, table->constants_vector[i].name, extra_roots);
	    //	
	    Val result = make_two_slot_record( task, TAGGED_INT_FROM_C_INT(id), name);
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
	    return result;	
	}
    }

    // Here, we did not find the constant:
    //
    Val name = make_ascii_string_from_c_string__may_heapclean (task, "<UNKNOWN>", extra_roots);
    //
    Val result = make_two_slot_record( task, TAGGED_INT_FROM_C_INT(-1), name);
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}

//
Val   dump_table_as_system_constants_list__may_heapclean   (Task* task,  Sysconsts* table,  Roots* extra_roots)   {
    //==================================================
    //
    // Generate a list of system constants from the given table.
    // We get called to list tables of signals, errors etc.

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);


    Val	result_list =  LIST_NIL;					Roots roots1 = { &result_list, extra_roots };


    for (int i = table->constants_count;  --i >= 0;  ) {
	//
	// If our agegroup0 buffer is more than half full,
	// empty it by doing a heapcleaning.  This is very
	// conservative -- which is the way I like it. :-)
	//
	if (agegroup0_freespace_in_bytes( task )
	  < agegroup0_usedspace_in_bytes( task )
	){
	    call_heapcleaner_with_extra_roots( task,  0, &roots1 );
	}

	Val name            =  make_ascii_string_from_c_string__may_heapclean(task, table->constants_vector[i].name, &roots1 );
	//
        Val system_constant =  make_two_slot_record( task, TAGGED_INT_FROM_C_INT(table->constants_vector[i].id), name);
	//
	result_list = LIST_CONS(  task, system_constant, result_list );
    }
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);

    return result_list;
}

//
Val   allocate_biwordslots_vector_sized_in_bytes__may_heapclean   (Task* task,  int nbytes,  Roots* extra_roots)   {	// This fn is NOWHERE INVOKED.
    //=========================================================
    //
    // Allocate a 64-bit aligned raw data chunk (to store abstract C data).
    //
    // This function is nowhere invoked.
    //
													ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Val result = allocate_biwordslots_vector__may_heapclean( task, (nbytes+7)>>2, extra_roots );		// Round size up to a multiple of sizeof(Int2) and dispatch.

													EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}													// 64-bit issue. (Is "+7)>>2" even correct? If so, should comment why.)


//
Val   make_biwordslots_vector_sized_in_bytes__may_heapclean   (Task* task,  void* data,  int nbytes,  Roots* extra_roots)   {
    //=====================================================
    //
    // Allocate a 64-bit aligned raw data chunk and initialize it to the given C data:

													ENTER_MYTHRYL_CALLABLE_C_FN(__func__);
													ramlog_printf("#%d  %s/AAA, nbytes d=%d\n",syscalls_seen,__func__, nbytes);
    Val result;

    if (nbytes == 0) {
	//
													ramlog_printf("#%d  %s/BBB, nbytes d=%d so returning HEAP_VOID\n",syscalls_seen,__func__, nbytes);
	result = HEAP_VOID;
	//
    } else {
	//
        result =  allocate_biwordslots_vector__may_heapclean( task, (nbytes +7) >> 2, NULL );		// Round size up to a multiple of sizeof(Int2).
													// 64-bit issue?
													ramlog_printf("#%d  %s/CCC, copying nbytes from data p=%d into result p=%p\n",syscalls_seen,__func__, nbytes,data,result);
	memcpy (PTR_CAST(void*, result), data, nbytes);
    }
													ramlog_printf("#%d  %s/ZZZ, result p=%p\n",syscalls_seen,__func__, result);
													EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.




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



