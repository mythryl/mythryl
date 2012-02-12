// make-strings-and-vectors-etc.c
//
// Support functions which mostly create various kinds of
// strings and vectors on the Mythryl heap.
//
// Multicore (pthread) note: when invoking the heapcleaner
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
// a loop because other pthreads may
// steal the memory before the checking
// pthread can use it.
																// sib_is_active		def in    src/c/h/heap.h
																// sib_freespace_in_bytes	def in    src/c/h/heap.h
 #if NEED_PTHREAD_SUPPORT
    //
    #define WHILE_INSUFFICIENT_FREESPACE_IN_SIB(ap, bytes)  while ((! sib_is_active(ap)) || (sib_freespace_in_bytes(ap) <= (bytes)))
 #else
    #define WHILE_INSUFFICIENT_FREESPACE_IN_SIB(ap, bytes)  if    ((! sib_is_active(ap)) || (sib_freespace_in_bytes(ap) <= (bytes)))
 #endif

#define COUNT_ALLOC(task, nbytes)	{	\
	Heap		*__h = task->heap;	\
	INCREASE_BIGCOUNTER(&(__h->total_bytes_allocated), (nbytes));	\
    }

//
Val   make_ascii_string_from_c_string__may_heapclean   (Task* task,  const char* v)   {
    //==============================================
    // 
    // Allocate a Mythryl string using a C string as an initializer.
    // We assume that the string is small and can be allocated in
    // the agegroup0 buffer.        XXX BUGGO FIXME

									    ENTER_MYTHRYL_CALLABLE_C_FN("make_ascii_string_from_c_string__may_heapclean");


    int len =   v == NULL  ?  0
                           :  strlen(v);

    if (len == 0) {
        //
	return ZERO_LENGTH_STRING__GLOBAL;
        //
    } else {
        //
	int	    n = BYTES_TO_WORDS(len+1);				// '+1' to count terminal '\0' also.

	Val result = allocate_nonempty_wordslots_vector__may_heapclean( task, n );

	// Zero the last word to allow fast (word) string comparisons,
	// and to guarantee 0 termination:
	//
	PTR_CAST( Val_Sized_Unt*, result) [n-1] = 0;
	strcpy (PTR_CAST(char*, result), v);

	return  make_vector_header(task, STRING_TAGWORD, result, len);
    }
}

//
Val   make_ascii_strings_from_vector_of_c_strings__may_heapclean   (Task *task, char **strs)   {
    //==========================================================
    // 
    // Given a NULL terminated rw_vector of char*, build a list of Mythryl strings.
    //
    // NOTE: we should do something about possible GC!!! XXX BUGGO FIXME**/


									    ENTER_MYTHRYL_CALLABLE_C_FN("make_ascii_strings_from_vector_of_c_strings__may_heapclean");

    int		i;
    Val	p, s;

    for (i = 0;  strs[i] != NULL;  i++)
	continue;

    p = LIST_NIL;
    while (i-- > 0) {
	s = make_ascii_string_from_c_string__may_heapclean(task, strs[i]);
	p = LIST_CONS(task, s, p);
    }

    return p;
}

//
Val   allocate_nonempty_ascii_string__may_heapclean   (Task* task,  int len)   {
    //=============================================
    // 
    // Allocate an uninitialized Mythryl string of length > 0.
    // This string is guaranteed to be padded to word size with 0 bytes,
    // and to be 0 terminated.

									    ENTER_MYTHRYL_CALLABLE_C_FN("allocate_nonempty_ascii_string__may_heapclean");

    int		nwords = BYTES_TO_WORDS(len+1);

    ASSERT(len > 0);

    Val result = allocate_nonempty_wordslots_vector__may_heapclean( task, nwords );

    // Zero the last word to allow fast (word) string comparisons,
    // and to guarantee 0 termination:
    //
    PTR_CAST(Val_Sized_Unt*, result)[nwords-1] = 0;

    return  make_vector_header(task,  STRING_TAGWORD, result, len);
}

//
Val   allocate_nonempty_wordslots_vector__may_heapclean   (Task* task,  int nwords)   {		// The "__may_heapclean" suffix is a warning to caller that the heapcleaner might be invoked in this fn,
    //=================================================						// potentially moving everything on the heap to a new address.
    // 
    // Allocate an uninitialized vector of word-sized slots.


										ENTER_MYTHRYL_CALLABLE_C_FN("allocate_nonempty_wordslots_vector__may_heapclean");

    Val	tagword = MAKE_TAGWORD(nwords, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG);
    Val	result;
    Val_Sized_Unt  bytesize;

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

	pthread_mutex_lock( &pth__mutex );								// Agegroup1 is shared with other pthreads, so grab the lock on it.
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
		    call_heapcleaner( task, 1 );
		    //
		pthread_mutex_lock( &pth__mutex );
                //
		sib->requested_extra_free_bytes = 0;
	    }

	    /////////////////////////////////////////////////////////	
	    // We now have enough sib space to create our vector.
	    // We cannot release the lock yet, because some other
	    // pthread might steal the freespace out from under us.
	    /////////////////////////////////////////////////////////	
	
	    *(sib->tospace.used_end++) = tagword;							// Lay down the vector tagword.
	    result = PTR_CAST( Val, sib->tospace.used_end);						// Save pointer to start of vector proper -- our return value.
	    sib->tospace.used_end += nwords;								// Allocate sibspace for tagword plus vector.	
	    //
	pthread_mutex_unlock( &pth__mutex );								// NOW we can safely release the heap lock.

	COUNT_ALLOC(task, bytesize);
    }

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

									    ENTER_MYTHRYL_CALLABLE_C_FN("shrink_fresh_wordslots_vector");

    int old_length_in_words = CHUNK_LENGTH(v);

    if (new_length_in_words == old_length_in_words)   return;

    ASSERT( new_length_in_words > 0  &&  new_length_in_words < old_length_in_words );

    if (old_length_in_words > MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS) {
        //
	Sib*  sib = task->heap->agegroup[ 0 ]->sib[ NONPTR_DATA_SIB ];

	ASSERT(sib->tospace.used_end - old_length_in_words == PTR_CAST(Val*, v)); 

	sib->tospace.used_end -= (old_length_in_words - new_length_in_words);

    } else {

	ASSERT(task->heap_allocation_pointer - old_length_in_words == PTR_CAST(Val*, v)); 
	task->heap_allocation_pointer -= (old_length_in_words - new_length_in_words);
    }

    PTR_CAST(Val*, v)[-1] = MAKE_TAGWORD(new_length_in_words, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG);
}

// Allocate an uninitialized chunk of raw64 data. 			I can't find any code which references this fn. -- 2011-10-25 CrT
// 
Val   allocate_biwordslots_vector__may_heapclean   (Task* task,  int nelems)   {
    //==========================================

									    ENTER_MYTHRYL_CALLABLE_C_FN("allocate_biwordslots_vector__may_heapclean");

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

	set_slot_in_nascent_heapchunk (task, 0, tagword);
	result = commit_nascent_heapchunk (task, nwords);

    } else {

	Sib* ap =   task->heap->agegroup[ 0 ]->sib[ NONPTR_DATA_SIB ];

	bytesize =  WORD_BYTESIZE*(nwords + 2);

	pthread_mutex_lock( &pth__mutex );
	    //
	    // NOTE: we use nwords+2 to allow for the alignment padding.

	    WHILE_INSUFFICIENT_FREESPACE_IN_SIB(ap, bytesize+task->heap_allocation_buffer_bytesize) {
		//
	        // We need to do a garbage collection:

		ap->requested_extra_free_bytes += bytesize;
		//
		pthread_mutex_unlock( &pth__mutex );
		    //
		    call_heapcleaner (task, 1);
		    //
		pthread_mutex_lock( &pth__mutex );
		//
		ap->requested_extra_free_bytes = 0;
	    }

	    #ifdef ALIGN_FLOAT64S
		//
		// Force FLOAT64_BYTESIZE alignment (tagword is off by one word)

	        #ifdef CHECK_HEAP
		    if (((Punt)ap->tospace.used_end & WORD_BYTESIZE) == 0) {
			*(ap->tospace.used_end) = (Val)0;
			++ap->tospace.used_end;
		    }
	        #else
		    ap->tospace.used_end = (Val *)(((Punt)ap->tospace.used_end) | WORD_BYTESIZE);
	        #endif
	    #endif

	    *ap->tospace.used_end ++
		=
		tagword;

	    result = PTR_CAST( Val, ap->tospace.used_end );

	    ap->tospace.used_end += nwords;

	pthread_mutex_unlock( &pth__mutex );

	COUNT_ALLOC(task, bytesize-WORD_BYTESIZE);
    }

    return result;
}

//
Val   allocate_nonempty_code_chunk   (Task* task,  int len)   {
    //============================
    //
    // Allocate an uninitialized Mythryl code chunk.
    // Assume that len > 1.

									    ENTER_MYTHRYL_CALLABLE_C_FN("allocate_nonempty_code_chunk");

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

    return PTR_CAST( Val, dp->chunk);
}

//
Val   allocate_nonempty_vector_of_one_byte_unts__may_heapclean   (Task* task,  int len)   {
    //========================================================
    // 
    // Allocate an uninitialized Lib7 bytearray.  Assume that len > 0.

									    ENTER_MYTHRYL_CALLABLE_C_FN("allocate_nonempty_vector_of_one_byte_unts__may_heapclean");

    int		nwords = BYTES_TO_WORDS(len);

    Val	result =  allocate_nonempty_wordslots_vector__may_heapclean( task, nwords );

    // Zero the last word to allow fast (word)
    // string comparisons, and to guarantee 0
    // termination:
    //
    PTR_CAST(Val_Sized_Unt*, result)[nwords-1] = 0;

    return  make_vector_header(task,  UNT8_RW_VECTOR_TAGWORD, result, len);
}

//
Val   allocate_nonempty_vector_of_eight_byte_floats__may_heapclean   (Task* task,  int len)   {
    //============================================================
    // 
    // Allocate an uninitialized Mythryl Float64 vector.  Assume that len > 0.

									    ENTER_MYTHRYL_CALLABLE_C_FN("allocate_nonempty_vector_of_eight_byte_floats__may_heapclean");

    Val result =  allocate_biwordslots_vector__may_heapclean( task, len );		// 64-bit issue.

    return make_vector_header( task,  FLOAT64_RW_VECTOR_TAGWORD, result, len );
}

//
Val   make_nonempty_rw_vector__may_heapclean   (Task* task,  int len,  Val init_val)   {
    //======================================
    // 
    // Allocate a Mythryl rw_vector using init_val
    // as the initial value for vector slots.
    // Assume that len > 0.

									    ENTER_MYTHRYL_CALLABLE_C_FN("make_nonempty_rw_vector__may_heapclean");

    Val	result;

    Val	tagword = MAKE_TAGWORD(len, RW_VECTOR_DATA_BTAG);


    Val_Sized_Unt	bytesize;

    if (len > MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS) {
        //
	Sib*	ap = task->heap->agegroup[ 0 ]->sib[ RW_POINTERS_SIB ];

	int	gc_level = (IS_POINTER(init_val) ? 0 : -1);

	bytesize = WORD_BYTESIZE*(len + 1);

	pthread_mutex_lock( &pth__mutex );

	    #if NEED_PTHREAD_SUPPORT
		clean_check: ;	// The pthread version jumps to here to recheck for GC.
	    #endif

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
	        // Clean heap -- but preserve init_val:
                //
		Val	root = init_val;
		ap->requested_extra_free_bytes += bytesize;
		pthread_mutex_unlock( &pth__mutex );
#ifndef OLDXTRAROOTS
		    call_heapcleaner_with_extra_roots (task, gc_level, &root, NULL);
#else
		    {   Roots r1 = { &root, NULL };
			//
			call_heapcleaner_with_extra_roots (task, gc_level, &r1 );
		    }
#endif
		    init_val = root;
		pthread_mutex_lock( &pth__mutex );
		ap->requested_extra_free_bytes = 0;

		#if NEED_PTHREAD_SUPPORT
		{
	            // Check again to insure that we have sufficient space:
		    gc_level = -1;
		    goto clean_check;
		}
		#endif
	    }
	    ASSERT(ap->tospace.used_end == ap->tospace.swept_end);
	    *(ap->tospace.used_end++) = tagword;
	    result = PTR_CAST( Val, ap->tospace.used_end);
	    ap->tospace.used_end += len;
	    ap->tospace.swept_end = ap->tospace.used_end;
	    //
	pthread_mutex_unlock( &pth__mutex );

	COUNT_ALLOC(task, bytesize);

    } else {

	set_slot_in_nascent_heapchunk (task, 0, tagword);
	result = commit_nascent_heapchunk (task, len);
    }

    Val* p = PTR_CAST(Val*, result);
    //
    for (int i = 0;  i < len; i++) {
	//
	*p++ = init_val;
    }

    return  make_vector_header(task,  TYPEAGNOSTIC_RW_VECTOR_TAGWORD, result, len);
}											// fun make_nonempty_rw_vector__may_heapclean

//
Val   make_nonempty_ro_vector__may_heapclean   (Task* task,  int len,  Val initializers)   {
    //======================================
    // 
    // Allocate a Mythryl vector, using the
    // list initializers as an initializer.
    // Assume that len > 0.
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("make_nonempty_ro_vector__may_heapclean");

    Val	tagword = MAKE_TAGWORD(len, RO_VECTOR_DATA_BTAG);
    Val* p;
    Val	result;

    if (len > MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS) {
	//
	// Since we want to avoid pointers from the
        // agegroup 1 record space into the agegroup0 space,
	// we need to do a cleaning (while preserving our
	// initializer list).

	Sib* 	ap = task->heap->agegroup[ 0 ]->sib[ RO_POINTERS_SIB ];

	Val	root = initializers;
	int	clean_level = 0;

	Val_Sized_Unt  bytesize
	    =
	    WORD_BYTESIZE * (len+1);

	pthread_mutex_lock( &pth__mutex );
	    //
	    if (! sib_is_active(ap)										// sib_is_active		def in    src/c/h/heap.h
		||
	        sib_freespace_in_bytes(ap) <= bytesize								// sib_freespace_in_bytes	def in    src/c/h/heap.h
                                              +
                                              task->heap_allocation_buffer_bytesize
	    ){
		clean_level = 1;
	    }

	    #if NEED_PTHREAD_SUPPORT
	        clean_check: ;			// The pthread version jumps to here to redo the garbage collection.
	    #endif

	    ap->requested_extra_free_bytes += bytesize;
	    pthread_mutex_unlock( &pth__mutex );
#ifndef OLDXTRAROOTS
	        call_heapcleaner_with_extra_roots (task, clean_level, &root, NULL);
#else
		{   Roots r1 = { &root, NULL };
		    //
		    call_heapcleaner_with_extra_roots (task, clean_level, &r1 );
		}
#endif
	        initializers = root;
	    pthread_mutex_lock( &pth__mutex );

	    ap->requested_extra_free_bytes = 0;

	    #if NEED_PTHREAD_SUPPORT
	    {   // Check again to ensure that we have sufficient space:
		//
		if (sib_freespace_in_bytes(ap) <= bytesize + task->heap_allocation_buffer_bytesize)   goto clean_check;
	    }
	    #endif

	    ASSERT(ap->tospace.used_end == ap->tospace.swept_end);
	    *(ap->tospace.used_end++) = tagword;
	    result = PTR_CAST( Val,  ap->tospace.used_end );
	    ap->tospace.used_end += len;
	    ap->tospace.swept_end = ap->tospace.used_end;
	    //
	pthread_mutex_unlock( &pth__mutex );

	COUNT_ALLOC(task, bytesize);

    } else {

	set_slot_in_nascent_heapchunk (task, 0, tagword);
	result = commit_nascent_heapchunk (task, len);
    }

    for (
	p = PTR_CAST(Val*, result);
	initializers != LIST_NIL;
	initializers = LIST_TAIL( initializers )
    ){
	*p++ = LIST_HEAD( initializers );
    }

    return  make_vector_header( task,  TYPEAGNOSTIC_RO_VECTOR_TAGWORD, result, len );
}													 // fun make_nonempty_ro_vector__may_heapclean

//
Val   make_system_constant__may_heapclean   (Task* task,  Sysconsts* table,  int id)   {
    //===================================
    // 
    // Find the system constant with the given id
    // in table, and allocate a pair to represent it.
    //
    // If the constant is not present then
    // return the pair (~1, "<UNKNOWN>").

									    ENTER_MYTHRYL_CALLABLE_C_FN("make_system_constant__may_heapclean");

    Val	name;

    for (int i = 0;  i < table->constants_count;  i++) {
	if (table->consts[i].id == id) {
	    name = make_ascii_string_from_c_string__may_heapclean (task, table->consts[i].name);
	    return make_two_slot_record( task, TAGGED_INT_FROM_C_INT(id), name);
	}
    }

    // Here, we did not find the constant:
    //
    name = make_ascii_string_from_c_string__may_heapclean (task, "<UNKNOWN>");
    return make_two_slot_record( task, TAGGED_INT_FROM_C_INT(-1), name);
}

//
Val   dump_table_as_system_constants_list__may_heapclean   (Task* task,  Sysconsts* table)   {
    //==================================================
    //
    // Generate a list of system constants from the given table.
    // We get called to list tables of signals, errors etc.

									    ENTER_MYTHRYL_CALLABLE_C_FN("dump_table_as_system_constants_list__may_heapclean");


    // Should check for available heap space !!! XXX BUGGO FIXME

    Val	result_list =  LIST_NIL;

    for (int i = table->constants_count;  --i >= 0;  ) {
	//
	Val name            =  make_ascii_string_from_c_string__may_heapclean (task, table->consts[i].name);
	//
        Val system_constant =  make_two_slot_record( task, TAGGED_INT_FROM_C_INT(table->consts[i].id), name);
	//
	result_list = LIST_CONS(  task, system_constant, result_list );
    }

    return result_list;
}

//
Val   allocate_biwordslots_vector_sized_in_bytes__may_heapclean   (Task* task,  int nbytes)   {
    //=========================================================
    //
    // Allocate a 64-bit aligned raw data chunk (to store abstract C data).
    //
    // This function is nowhere invoked.
    //
    return  allocate_biwordslots_vector__may_heapclean( task, (nbytes+7)>>2 );		// Round size up to a multiple of sizeof(Int2) and dispatch.
}


//
Val   make_biwordslots_vector_sized_in_bytes__may_heapclean   (Task* task,  void* data,  int nbytes)   {
    //=====================================================
    //
    // Allocate a 64-bit aligned raw data chunk and initialize it to the given C data:

									    ENTER_MYTHRYL_CALLABLE_C_FN("make_biwordslots_vector_sized_in_bytes__may_heapclean");

    if (nbytes == 0) {

	return HEAP_VOID;

    } else {

        Val chunk =  allocate_biwordslots_vector__may_heapclean( task, (nbytes +7) >> 2 );	// Round size up to a multiple of sizeof(Int2).
												// 64-bit issue?
	memcpy (PTR_CAST(void*, chunk), data, nbytes);

	return chunk;
    }
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



