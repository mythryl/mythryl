// make-strings-and-vectors-etc.c
//
// Support functions which mostly create various kinds of
// strings and vectors on the Mythryl heap.
//
// MP Note: when invoking the garbage collector, we add the
// requested size to requested_sib_buffer_bytesize, so that multiple processors
// can request space at the same time.


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

#include "runtime-base.h"
#include "heap.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-configuration.h"
#include <string.h>


// A cleaning-needed check.
// On multicore systems this needs to be
// a loop because other pthreads may
// steal the memory before the checking
// pthread can use it.
																// sib_is_active		def in    src/c/h/heap.h
																// sib_freespace_in_bytes	def in    src/c/h/heap.h
 #if NEED_PTHREAD_SUPPORT
    //
    #define IFGC(ap, szb)  while ((! sib_is_active(ap)) || (sib_freespace_in_bytes(ap) <= (szb)))
 #else
    #define IFGC(ap, szb)  if    ((! sib_is_active(ap)) || (sib_freespace_in_bytes(ap) <= (szb)))
 #endif

#define COUNT_ALLOC(task, nbytes)	{	\
	Heap		*__h = task->heap;	\
	INCREASE_BIGCOUNTER(&(__h->total_bytes_allocated), (nbytes));	\
    }


Val   make_ascii_string_from_c_string   (Task* task,  const char* v)   {
    //===============================
    // 
    // Allocate a Mythryl string using a C string as an initializer.
    // We assume that the string is small and can be allocated in
    // the agegroup0 buffer.        XXX BUGGO FIXME

    int len =   v == NULL  ?  0
                           :  strlen(v);

    if (len == 0) {
	return ZERO_LENGTH_STRING__GLOBAL;
    } else {

	int	    n = BYTES_TO_WORDS(len+1);				// Count "\0" too.

	Val result = allocate_nonempty_int1_vector( task, n );

	// Zero the last word to allow fast (word) string comparisons,
	// and to guarantee 0 termination:
	//
	PTR_CAST( Val_Sized_Unt*, result) [n-1] = 0;
	strcpy (PTR_CAST(char*, result), v);

	SEQHDR_ALLOC (task, result, STRING_TAGWORD, result, len);

	return result;
    }
}


Val   make_ascii_strings_from_vector_of_c_strings   (Task *task, char **strs)   {
    //===================================================
    // 
    // Given a NULL terminated rw_vector of char*, build a list of Mythryl strings.
    //
    // NOTE: we should do something about possible GC!!! XXX BUGGO FIXME**/

    int		i;
    Val	p, s;

    for (i = 0;  strs[i] != NULL;  i++)
	continue;

    p = LIST_NIL;
    while (i-- > 0) {
	s = make_ascii_string_from_c_string(task, strs[i]);
	LIST_CONS(task, p, s, p);
    }

    return p;
}


Val   allocate_nonempty_ascii_string   (Task* task,  int len)   {
    //==============================
    // 
    // Allocate an uninitialized Mythryl string of length > 0.
    // This string is guaranteed to be padded to word size with 0 bytes,
    // and to be 0 terminated.

    int		nwords = BYTES_TO_WORDS(len+1);

    ASSERT(len > 0);

    Val result = allocate_nonempty_int1_vector( task, nwords );

    // Zero the last word to allow fast (word) string comparisons,
    // and to guarantee 0 termination:
    //
    PTR_CAST(Val_Sized_Unt*, result)[nwords-1] = 0;

    SEQHDR_ALLOC (task, result/*=*/, STRING_TAGWORD, result, len);

    return result;
}


Val   allocate_nonempty_int1_vector   (Task* task,  int nwords)   {
    //==============================
    // 
    // Allocate an uninitialized vector of 32-bit slots.

    Val	tagword = MAKE_TAGWORD(nwords, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG);
    Val	result;
    Val_Sized_Unt	bytesize;

    ASSERT(nwords > 0);

    if (nwords <= MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS) {
	//
	LIB7_AllocWrite (task, 0, tagword);
	result = LIB7_Alloc (task, nwords);

    } else {

	Sib* ap =   task->heap->agegroup[ 0 ]->sib[ STRING_ILK ];

	bytesize = WORD_BYTESIZE*(nwords + 1);

	PTH__MUTEX_LOCK( &pth__heapcleaner_gen_mutex__global );
	    //
	    IFGC (ap, bytesize+task->heap->agegroup0_buffer_bytesize) {

	        // We need to do a garbage collection:
                //
		ap->requested_sib_buffer_bytesize += bytesize;
                //
		PTH__MUTEX_UNLOCK( &pth__heapcleaner_gen_mutex__global );
		    //
		    call_heapcleaner( task, 1 );
		    //
		PTH__MUTEX_LOCK( &pth__heapcleaner_gen_mutex__global );
                //
		ap->requested_sib_buffer_bytesize = 0;
	    }
	    *(ap->next_tospace_word_to_allocate++) = tagword;
	    result = PTR_CAST( Val, ap->next_tospace_word_to_allocate);
	    ap->next_tospace_word_to_allocate += nwords;

	PTH__MUTEX_UNLOCK( &pth__heapcleaner_gen_mutex__global );

	COUNT_ALLOC(task, bytesize);

    }

    return result;
}

void   shrink_fresh_int1_vector   (Task* task,  Val v,  int new_length_in_words)   {
    // =========================
    // 
    // Shrink a freshly allocated one_word_int vector.
    // This is used by the input routines that must pessimistically
    // pre-allocate space for more input than actually gets read.

    int old_length_in_words = CHUNK_LENGTH(v);

    if (new_length_in_words == old_length_in_words)   return;

    ASSERT( new_length_in_words > 0  &&  new_length_in_words < old_length_in_words );

    if (old_length_in_words > MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS) {
        //
	Sib*  ap = task->heap->agegroup[ 0 ]->sib[ STRING_ILK ];

	ASSERT(ap->next_tospace_word_to_allocate - old_length_in_words == PTR_CAST(Val*, v)); 

	ap->next_tospace_word_to_allocate -= (old_length_in_words - new_length_in_words);

    } else {

	ASSERT(task->heap_allocation_pointer - old_length_in_words == PTR_CAST(Val*, v)); 
	task->heap_allocation_pointer -= (old_length_in_words - new_length_in_words);
    }

    PTR_CAST(Val*, v)[-1] = MAKE_TAGWORD(new_length_in_words, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG);
}

// Allocate an uninitialized chunk of raw64 data. 			I can't find any code which references this fn. -- 2011-10-25 CrT
// 
Val   allocate_int2_vector   (Task* task,  int nelems)   {
    //==================== 

    int	nwords = DOUBLES_TO_WORDS(nelems);
    Val	tagword   = MAKE_TAGWORD(nwords, EIGHT_BYTE_ALIGNED_NONPOINTER_DATA_BTAG);
    Val	result;
    Val_Sized_Unt	bytesize;

    if (nwords <= MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS) {

	#ifdef ALIGN_FLOAT64S
	    // Force FLOAT64_BYTESIZE alignment:
	    //
	    task->heap_allocation_pointer = (Val *)((Punt)(task->heap_allocation_pointer) | WORD_BYTESIZE);
	#endif

	LIB7_AllocWrite (task, 0, tagword);
	result = LIB7_Alloc (task, nwords);

    } else {

	Sib* ap =   task->heap->agegroup[ 0 ]->sib[ STRING_ILK ];

	bytesize =  WORD_BYTESIZE*(nwords + 2);

	PTH__MUTEX_LOCK( &pth__heapcleaner_gen_mutex__global );
	    //
	    // NOTE: we use nwords+2 to allow for the alignment padding.

	    IFGC (ap, bytesize+task->heap->agegroup0_buffer_bytesize) {
		//
	        // We need to do a garbage collection:

		ap->requested_sib_buffer_bytesize += bytesize;
		//
		PTH__MUTEX_UNLOCK( &pth__heapcleaner_gen_mutex__global );
		    //
		    call_heapcleaner (task, 1);
		    //
		PTH__MUTEX_LOCK( &pth__heapcleaner_gen_mutex__global );
		//
		ap->requested_sib_buffer_bytesize = 0;
	    }
	    #ifdef ALIGN_FLOAT64S
		//
		// Force FLOAT64_BYTESIZE alignment (tagword is off by one word)

	        #ifdef CHECK_HEAP
		    if (((Punt)ap->next_tospace_word_to_allocate & WORD_BYTESIZE) == 0) {
			*(ap->next_tospace_word_to_allocate) = (Val)0;
			ap->next_tospace_word_to_allocate++;
		    }
	        #else
		    ap->next_tospace_word_to_allocate = (Val *)(((Punt)ap->next_tospace_word_to_allocate) | WORD_BYTESIZE);
	        #endif
	    #endif

	    *ap->next_tospace_word_to_allocate ++
		=
		tagword;

	    result = PTR_CAST( Val, ap->next_tospace_word_to_allocate );

	    ap->next_tospace_word_to_allocate += nwords;

	PTH__MUTEX_UNLOCK( &pth__heapcleaner_gen_mutex__global );

	COUNT_ALLOC(task, bytesize-WORD_BYTESIZE);
    }

    return result;
}


Val   allocate_nonempty_code_chunk   (Task* task,  int len)   {
    //============================
    //
    // Allocate an uninitialized Mythryl code chunk.
    // Assume that len > 1.

    Heap*  heap = task->heap;

    int    allocGen = (heap->active_agegroups < CODECHUNK_ALLOCATION_AGEGROUP)
			? heap->active_agegroups
			: CODECHUNK_ALLOCATION_AGEGROUP;

    Agegroup* age =   heap->agegroup[ allocGen-1 ];

    Hugechunk* dp;

    PTH__MUTEX_LOCK( &pth__heapcleaner_gen_mutex__global );
	//
	dp = allocate_hugechunk (heap, allocGen, len);
	ASSERT(dp->gen == allocGen);
	dp->next = age->hugechunks[ CODE__HUGE_ILK ];
	age->hugechunks[ CODE__HUGE_ILK ] = dp;
	dp->huge_ilk = CODE__HUGE_ILK;
	COUNT_ALLOC(task, len);
	//
    PTH__MUTEX_UNLOCK( &pth__heapcleaner_gen_mutex__global );

    return PTR_CAST( Val, dp->chunk);
}


Val   allocate_nonempty_vector_of_one_byte_unts   (Task* task,  int len)   {
    //=========================================
    // 
    // Allocate an uninitialized Lib7 bytearray.  Assume that len > 0.

    int		nwords = BYTES_TO_WORDS(len);

    Val	result =  allocate_nonempty_int1_vector( task, nwords );

    // Zero the last word to allow fast (word)
    // string comparisons, and to guarantee 0
    // termination:
    //
    PTR_CAST(Val_Sized_Unt*, result)[nwords-1] = 0;

    SEQHDR_ALLOC (task, result, UNT8_RW_VECTOR_TAGWORD, result, len);

    return result;
}


Val   allocate_nonempty_vector_of_eight_byte_floats   (Task* task,  int len)   {
    //================================
    // 
    // Allocate an uninitialized Mythryl Float64 vector.  Assume that len > 0.

    Val result =  allocate_int2_vector( task, len );

    SEQHDR_ALLOC( task, result, FLOAT64_RW_VECTOR_TAGWORD, result, len );

    return result;
}


Val   make_nonempty_rw_vector   (Task* task,  int len,  Val init_val)   {
    //=======================
    // 
    // Allocate a Mythryl rw_vector using init_val
    // as the initial value for vector slots.
    // Assume that len > 0.

    Val	result;

    Val	tagword = MAKE_TAGWORD(len, RW_VECTOR_DATA_BTAG);


    Val_Sized_Unt	bytesize;

    if (len > MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS) {
        //
	Sib*	ap = task->heap->agegroup[ 0 ]->sib[ VECTOR_ILK ];

	int	gc_level = (IS_POINTER(init_val) ? 0 : -1);

	bytesize = WORD_BYTESIZE*(len + 1);
													// pth__heapcleaner_gen_mutex__global	def in   src/c/pthread/pthread-on-posix-threads.c
													// 				or       src/c/pthread/pthread-on-sgi.c
													// 				or	 src/c/pthread/pthread-on-solaris.c
													// (Used only in this file.)

	PTH__MUTEX_LOCK( &pth__heapcleaner_gen_mutex__global );						// pth__mutex_lock		def in   src/c/h/runtime-base.h
	    //												// as pth__mutex_lock(lock)	from	 src/c/pthread/pthread-on-posix-threads.c
	    //												//				or	 src/c/pthread/pthread-on-sgi.c
	    #if NEED_PTHREAD_SUPPORT									//				or	 src/c/pthread/pthread-on-solaris.c
		clean_check: ;	// The pthread version jumps to here to recheck for GC.
	    #endif

	    if (! sib_is_active(ap)									// sib_is_active		def in    src/c/h/heap.h
		||
	        sib_freespace_in_bytes(ap) <= bytesize							// sib_freespace_in_bytes	def in    src/c/h/heap.h
                                              +
                                              task->heap->agegroup0_buffer_bytesize
            ){
		gc_level = 1;
	    }

	    if (gc_level >= 0) {
		//
	        // Clean heap -- but preserve init_val:
                //
		Val	root = init_val;
		ap->requested_sib_buffer_bytesize += bytesize;
		PTH__MUTEX_UNLOCK( &pth__heapcleaner_gen_mutex__global );
		    call_heapcleaner_with_extra_roots (task, gc_level, &root, NULL);
		    init_val = root;
		PTH__MUTEX_LOCK( &pth__heapcleaner_gen_mutex__global );
		ap->requested_sib_buffer_bytesize = 0;

		#if NEED_PTHREAD_SUPPORT
		if (pth__done_pthread_create__global) {
		    //
	            // Check again to insure that we have sufficient space:
		    gc_level = -1;
		    goto clean_check;
		}
		#endif
	    }
	    ASSERT(ap->next_tospace_word_to_allocate == ap->next_word_to_sweep_in_tospace);
	    *(ap->next_tospace_word_to_allocate++) = tagword;
	    result = PTR_CAST( Val, ap->next_tospace_word_to_allocate);
	    ap->next_tospace_word_to_allocate += len;
	    ap->next_word_to_sweep_in_tospace = ap->next_tospace_word_to_allocate;
	    //
	PTH__MUTEX_UNLOCK( &pth__heapcleaner_gen_mutex__global );

	COUNT_ALLOC(task, bytesize);

    } else {

	LIB7_AllocWrite (task, 0, tagword);
	result = LIB7_Alloc (task, len);
    }

    Val* p = PTR_CAST(Val*, result);
    //
    for (int i = 0;  i < len; i++) {
	//
	*p++ = init_val;
    }

    SEQHDR_ALLOC (task, result, TYPEAGNOSTIC_RW_VECTOR_TAGWORD, result, len);

    return result;
}							// fun make_nonempty_rw_vector


Val   make_nonempty_ro_vector   (Task* task,  int len,  Val initializers)   {
    //======================= 
    // 
    // Allocate a Mythryl vector, using the
    // list initializers as an initializer.
    // Assume that len > 0.
    //
    Val	tagword = MAKE_TAGWORD(len, RO_VECTOR_DATA_BTAG);
    Val* p;
    Val	result;

    if (len > MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS) {
	//
	// Since we want to avoid pointers from the
        // agegroup 1 record space into the agegroup0 space,
	// we need to do a cleaning (while perserving our
	// initializer list).

	Sib* 	ap = task->heap->agegroup[ 0 ]->sib[ RECORD_ILK ];

	Val	root = initializers;
	int	clean_level = 0;

	Val_Sized_Unt  bytesize
	    =
	    WORD_BYTESIZE * (len+1);

	PTH__MUTEX_LOCK( &pth__heapcleaner_gen_mutex__global );
	    //
	    if (! sib_is_active(ap)										// sib_is_active		def in    src/c/h/heap.h
		||
	        sib_freespace_in_bytes(ap) <= bytesize							// sib_freespace_in_bytes	def in    src/c/h/heap.h
                                              +
                                              task->heap->agegroup0_buffer_bytesize
	    ){
		clean_level = 1;
	    }

	    #if NEED_PTHREAD_SUPPORT
	        clean_check: ;			// The pthread version jumps to here to redo the garbage collection.
	    #endif

	    ap->requested_sib_buffer_bytesize += bytesize;
	    PTH__MUTEX_UNLOCK( &pth__heapcleaner_gen_mutex__global );
	        call_heapcleaner_with_extra_roots (task, clean_level, &root, NULL);
	        initializers = root;
	    PTH__MUTEX_LOCK( &pth__heapcleaner_gen_mutex__global );

	    ap->requested_sib_buffer_bytesize = 0;

	    #if NEED_PTHREAD_SUPPORT
	    if (pth__done_pthread_create__global) {
		//
	        // Check again to ensure that we have sufficient space:
		//
		if (sib_freespace_in_bytes(ap) <= bytesize + task->heap->agegroup0_buffer_bytesize)   goto clean_check;
	    }
	    #endif

	    ASSERT(ap->next_tospace_word_to_allocate == ap->next_word_to_sweep_in_tospace);
	    *(ap->next_tospace_word_to_allocate++) = tagword;
	    result = PTR_CAST( Val,  ap->next_tospace_word_to_allocate );
	    ap->next_tospace_word_to_allocate += len;
	    ap->next_word_to_sweep_in_tospace = ap->next_tospace_word_to_allocate;
	    //
	PTH__MUTEX_UNLOCK( &pth__heapcleaner_gen_mutex__global );

	COUNT_ALLOC(task, bytesize);

    } else {

	LIB7_AllocWrite (task, 0, tagword);
	result = LIB7_Alloc (task, len);
    }

    for (
	p = PTR_CAST(Val*, result);
	initializers != LIST_NIL;
	initializers = LIST_TAIL( initializers )
    ){
	*p++ = LIST_HEAD( initializers );
    }

    SEQHDR_ALLOC( task, result, TYPEAGNOSTIC_RO_VECTOR_TAGWORD, result, len );

    return result;
}						 // fun make_nonempty_ro_vector


Val   make_system_constant   (Task* task,  System_Constants_Table* table,  int id)   {
    //====================
    // 
    // Find the system constant with the given id
    // in table, and allocate a pair to represent it.
    //
    // If the constant is not present then
    // return the pair (~1, "<UNKNOWN>").

    Val	name;
    Val result;

    for (int i = 0;  i < table->constants_count;  i++) {
	if (table->consts[i].id == id) {
	    name = make_ascii_string_from_c_string (task, table->consts[i].name);
	    REC_ALLOC2 (task, result, TAGGED_INT_FROM_C_INT(id), name);
	    return result;
	}
    }

    // Here, we did not find the constant:
    //
    name = make_ascii_string_from_c_string (task, "<UNKNOWN>");
    REC_ALLOC2 (task, result, TAGGED_INT_FROM_C_INT(-1), name);
    return result;
}


Val   dump_table_as_system_constants_list   (Task* task,  System_Constants_Table* table)   {
    //===================================
    //
    // Generate a list of system constants from the given table.
    // We get called to list tables of signals, errors etc.


    // Should check for available heap space !!! XXX BUGGO FIXME

    Val	result_list =  LIST_NIL;

    for (int i = table->constants_count;  --i >= 0;  ) {
	//
	Val name = make_ascii_string_from_c_string (task, table->consts[i].name);
        Val                            system_constant;
	REC_ALLOC2( task,              system_constant, TAGGED_INT_FROM_C_INT(table->consts[i].id), name);
	LIST_CONS(  task, result_list, system_constant, result_list );
    }

    return result_list;
}


Val   allocate_int2_vector_sized_in_bytes   (Task* task,  int nbytes)   {
    //====================================
    //
    // Allocate a 64-bit aligned raw data chunk (to store abstract C data).
    //
    // This function is nowhere invoked.
    //
    return  allocate_int2_vector( task, (nbytes+7)>>2 );		// Round size up to a multiple of sizeof(Int2) and dispatch.
}



Val   make_int2_vector_sized_in_bytes   (Task* task,  void* data,  int nbytes)   {
    //================================
    //
    // Allocate a 64-bit aligned raw data chunk and initialize it to the given C data:

    if (nbytes == 0) {

	return HEAP_VOID;

    } else {

        Val chunk =  allocate_int2_vector( task, (nbytes +7) >> 2 );	// Round size up to a multiple of sizeof(Int2).

	memcpy (PTR_CAST(void*, chunk), data, nbytes);

	return chunk;
    }
}


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


