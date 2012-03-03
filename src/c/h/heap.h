// heap.h
//
// Mythryl-heap-related types, functions, macros and constants.


/*
###              "It is only with the heart one can see clearly;
###               what is essential is invisible to the eye."
###
###                             -- Antoine de Saint-Exupery
*/


// 'quire' (pronounced like choir) designates a multipage ram region
//         allocated directly from the host operating system.
//
//     quire: 2. A collection of leaves of parchment or paper,
//               folded one within the other, in a manuscript or book.
//                     -- http://www.thefreedictionary.com/quire

#ifndef HEAP_H
#define HEAP_H

#include "runtime-base.h"
#include "heapcleaner.h"
#include "sibid.h"
#include "heap-tags.h"

#ifndef OBTAIN_QUIRE_FROM_OS_H
    typedef   struct quire
                     Quire;
#endif

#ifndef INTER_AGEGROUP_POINTERS_MAP_H
    typedef  struct coarse_inter_agegroup_pointers_map
                    Coarse_Inter_Agegroup_Pointers_Map;
#endif

#include "bigcounter.h"


struct cleaner_args {		// "typedef   struct cleaner_args_rec   Heapcleaner_Args;"   in   src/c/h/runtime-base.h
    //
    Punt agegroup0_buffer_bytesize;
    int	 active_agegroups;
    int  oldest_agegroup_retaining_fromspace_sibs_between_heapcleanings;		// Between garbage collections we keep (instead of freeing) idle fromspaces for this and all younger agegroups.
											// For more background, see comments on DEFAULT_OLDEST_AGEGROUP_RETAINING_FROMSPACE_SIBS_BETWEEN_HEAPCLEANINGS in src/c/h/runtime-configuration.h
};


// Forward declarations to enable mutual recursion:
//
typedef   struct repair            Repair;						// Defined below.
typedef   struct sib               Sib;							// Defined below.
typedef   struct hugechunk_quire  Hugechunk_Quire;					// Defined below.
typedef   struct hugechunk  	   Hugechunk;						// Defined below.
typedef   struct agegroup          Agegroup;						// Defined below.

/* typedef   struct heap   Heap; */							// From  src/c/h/runtime-base.h



											// Quire		def in    src/c/h/get-quire-from-os.h
											// struct quire	def in    src/c/ram/get-quire-from-mmap.c
											// struct quire	def in    src/c/ram/get-quire-from-mach.c
											// struct quire	def in    src/c/ram/get-quire-from-win32.c


// A heap consists of one agegroup0 buffer per pthread
// plus one or more older agegroups, which are shared
// between all pthreads.
//
struct heap {
    Val*		agegroup0_master_buffer;					// Base address of the master buffer from which we allocate the individual per-task agegroup0 buffers.
    Punt		agegroup0_master_buffer_bytesize;				// Size-in-bytes of the agegroup0_buffers master buffer.

    Quire*		quire;								// The memory region we got from the host OS to contain the book_to_sibid map and agegroup0 buffer(s).

    int			active_agegroups;						// Number of active agegroups. (Not including agegroup0.)  Typically 5 -- see DEFAULT_ACTIVE_AGEGROUPS in src/c/h/runtime-configuration.h
											// This value is never changed once set in src/c/heapcleaner/heapcleaner-initialization.c

    int			agegroup0_heapcleanings_count;					// Count of how many times we've heapcleaned ("garbage-collected") heap agegroup zero.

    int			oldest_agegroup_retaining_fromspace_sibs_between_heapcleanings;	// Between heapcleanings retain the from-space buffer (only) for agegroups 1..oldest_agegroup_retaining_fromspace_sibs_between_heapcleanings.
											// For more background, see comments on DEFAULT_OLDEST_AGEGROUP_RETAINING_FROMSPACE_SIBS_BETWEEN_HEAPCLEANINGS in src/c/h/runtime-configuration.h

    Agegroup*	        agegroup[ MAX_AGEGROUPS ];					// Age-group #i is in agegroup[i-1]

    int		        hugechunk_quire_count;						// Number of active hugechunk quires.
    Hugechunk_Quire*	hugechunk_quires;						// List of hugechunk quires.
    Hugechunk*		hugechunk_freelist;						// Freelist header for hugechunks.

    Val*		weakrefs_forwarded_during_heapcleaning;				// List of weakrefs forwarded during heapcleaning.
											// This is really local state for the heapcleaner -- it doesn't belong here.  XXX SUCKO FIXME.
    //
    Bigcounter		total_bytes_allocated;						// Cleaner statistics -- tracks number of bytes  allocated.
    Bigcounter		total_bytes_copied_to_sib[ MAX_AGEGROUPS ][ MAX_PLAIN_SIBS ];	// Cleaner statistics -- tracks number of bytes copied into each sib buffer.
};




// This macro answers the question:
// What value should we set the heap_allocation_limit
// to for a given heap allocation buffer?
//
#define HEAP_ALLOCATION_LIMIT(task)			\
    HEAP_ALLOCATION_LIMIT_SIZE((task)->heap_allocation_buffer,(task)->heap_allocation_buffer_bytesize)

// This macro is mostly private support for the above macro,
// although it is occasionally used by client code:
//
#define HEAP_ALLOCATION_LIMIT_SIZE(base,size)	\
    (Val*)((Punt)(base) + (size) - (MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER + AGEGROUP0_OVERRUN_TRIPWIRE_BUFFER_SIZE_IN_BYTES))


// An age-group:
//
struct agegroup {
    //
    Heap*   heap;						// A back pointer to the heap record.
    int	    age;						// Which agegroup this is (1..active_agegroups).
    int	    heapcleanings_count;				// Number of times this agegroup has been heapcleaned.
    int	    target_heapcleaning_frequency_ratio;		// Desired number of collections of the previous agegroup for one collection of this agegroup.

    int	    heapcleanings_count_of_younger_agegroup_during_last_heapcleaning;	// Number cleanings of the previous (younger) agegroup last time this agegroup was cleaned.
										// (We use this to check how close we are coming to desired ratio of heapcleanings between agegroups.)

    Sib*    sib[ MAX_PLAIN_SIBS ];				// MAX_PLAIN_SIBS		def in    src/c/h/sibid.h

    Hugechunk*   hugechunks[ MAX_HUGE_SIBS ];			// MAX_HUGE_SIBS		def in    src/c/h/sibid.h

    Quire*    tospace_quire;					// The host-OS multipage ram regions that this agegroup is
    Quire*    fromspace_quire;					// using for the to-space and from-space.
    Quire*    retained_fromspace_quire;				// For younger (== smaller) agegroups, we retain the from-space quire between heapcleanings, instead of giving it back to the host OS.
    //
    Coarse_Inter_Agegroup_Pointers_Map*				// Coarse_Inter_Agegroup_Pointers_Map	def in   src/c/h/coarse-inter-agegroup-pointers-map.h
    coarse_inter_agegroup_pointers_map;				// The dirty cards in the vector sib for this agegroup.
};


// Each heap agegroup > 0 has four sib buffers, one each to hold
//
//     rw_conscells   (Blocks of immutable pointers of     length     two.)
//     ro_pointers    (Blocks of immutable pointers of any length but two.)
//     nonptr_data    (Strings, int32 vector etc. Mutable and immutable both.)
//     rw_pointers    (Refcells and rw_vectors of pointers.)
//
// While the heapclean-n-agegroups.c code is running
// will actually have two buffers per sib:
//   one for from-space,
//   one for to-space.
// Otherwise, we sometimes retain the (unused) to-space
// buffer between heapcleanings, and sometimes free()
// it, to be malloc()'d again on the next heapclean:
//
//
struct sib {
    //
    Sibid		id;					// The to-space version of this sib's identifier.

    struct tospace {
	Val*		used_end;				// The next word to allocate in this sib's to-space.
	//
	Val*	        start;					// Base address of to-space.
	Punt		bytesize;				// Size of to-space.
	Val*	        limit;					// The end of to-space (tospace.start+tospace.bytesize).
	//
	Val*		swept_end;				// State variable used (only) during heapcleaning.  During heapcleaning
								// we treat to-space as a work queue, with this pointer marking the
								// the start of the queue and the end-of-tospace pointer the end
								// -- see src/c/heapcleaner/heapclean-n-agegroups.c.
								//
								// The critical invariant is that chunks before this pointer (i.e.,
								// fully processed chunks) contain only pointers into to-space, while
								// chunks after this pointer contain only pointers into from-space.
    } tospace;

    struct fromspace {
	Val*		start;					// Base address of from-space.
	Vunt	bytesize;				// Size of from-space.
	Val*		used_end;				// The end of the used portion of from-space.

	Val*		seniorchunks_end;				// We require that a chunk survive two heapcleans in a
								// given agegroup before being promoted to the next agegroup.
								// To this end we divide the chunks in a given agegroup sib into
								// "junior" (have not yet survived a heapclean) and
								// "senior" (have survived one heapclean).  This pointer tracks the
								// boundary between senior and junior;  Chunks before this get promoted
								// if they survive the next heapcleaning; those beyond it do not.
								// Special case: chunks in the oldest active agegroup are forever junior.
    } fromspace;

    Sib*		sib_for_promoted_chunks;		// Next older sib, except for oldest sib, which points to itself.

    // Heap sizing parameters:
    //
    Vunt	requested_extra_free_bytes;		// Requested minimum size for this sib buffer. (This is in addition to the required minimum size.)
								// Normally zero.  This is used to reserve sufficient extra space for vectors (etc) being created by fns in
								//     src/c/heapcleaner/make-strings-and-vectors-etc.c
								// This is essentially a hidden extra parameter to call_heapcleaner() from these fns.

    Vunt	soft_max_bytesize;			// A soft maximum size for this sib buffer.



    // ======================================================== //
    Bool		heap_needs_repair;			// Set to TRUE when exporting if the sib had external references that require repair.
								// This is basically local state for src/c/heapcleaner/export-heap.c + src/c/heapcleaner/datastructure-pickler-cleaner.c
								// -- it SHOULD NOT BE HERE.  XXX SUCKO FIXME.
    Repair*		repairlist;				// Points to the top of the repair list (for pickling datastructures).
								// The repair list grows  down in to-space.
								// This is basically local state for src/c/heapcleaner/datastructure-pickler-cleaner.c
								// -- it SHOULD NOT BE HERE.  XXX SUCKO FIXME.
    // ======================================================== //
};

//
inline void   make_sib_tospace_into_fromspace   (Sib* sib)   {
    //        ===============================
    //
    // Make to-space into from-space:
    //
    sib->fromspace.start	=  sib->tospace.start;
    sib->fromspace.bytesize	=  sib->tospace.bytesize;
    sib->fromspace.used_end	=  sib->tospace.used_end;
}

//
inline Bool   sib_is_active   (Sib* sib)   {
    //        =============
    //
    // Return TRUE iff this sib has an allocated to-space:
    //
    return  sib->tospace.bytesize  >  0;
}

// 
inline Bool   sib_chunk_is_senior   (Sib* sib,  Val* pointer)   {
    //        ===================
    //
    // Return TRUE iff 'pointer' is in
    // the 'senior chunks' segment of this sib:
    //
    return   pointer < sib->fromspace.seniorchunks_end;
}

//
inline Punt   sib_freespace_in_bytes   (Sib* sib)   {
    //        ======================
    //
    // Return the amount of free space
    // (in bytes) available in a sib buffer:
    //
    return  (Punt)  sib->tospace.limit
          - (Punt)  sib->tospace.used_end;
}

//
inline Punt   sib_space_used_in_bytes   (Sib* sib)   {
    //        =======================
    //
    // Return the amount of allocated space
    // (in bytes) in a sib buffer:
    //
    return   (Punt)   sib->tospace.used_end
             -
             (Punt)   sib->tospace.start;
}

//
inline Punt   agegroup0_freespace_in_bytes   (Task* task)   {
    //        ============================
    //
    // Return the amount of free space (in bytes)
    // available in the agegroup0 buffer for this task:
    //
    return  (Punt)  task->real_heap_allocation_limit
          - (Punt)  task->heap_allocation_pointer;
}

//
inline Punt   agegroup0_freespace_in_words   (Task* task)   {
    //        ============================
    //
    // Return the amount of free space (in words)
    // available in the agegroup0 buffer for this task:
    //
    return  task->real_heap_allocation_limit
          - task->heap_allocation_pointer;
}

//
inline Punt   agegroup0_usedspace_in_bytes   (Task* task)   {
    //        ============================
    //
    // Return the amount of space used (in bytes)
    // in the agegroup0 buffer for this task:
    //
    return  (Punt)  task->heap_allocation_pointer
          - (Punt)  task->heap_allocation_buffer;
}

//
inline Punt   agegroup0_usedspace_in_words   (Task* task)   {
    //        ============================
    //
    // Return the amount of space used (in words)
    // in the agegroup0 buffer for this task:
    //
    return  task->heap_allocation_pointer
          - task->heap_allocation_buffer;
}

//
inline Punt   agegroup0_buffer_size_in_bytes   (Task* task)   {
    //        ==============================
    //
    // Return the size-in-bytes of the
    // agegroup0 buffer for this task:
    //
    return  (Punt)  task->real_heap_allocation_limit
          - (Punt)  task->heap_allocation_buffer;
}

//
inline Punt   agegroup0_buffer_size_in_words   (Task* task)   {
    //        ==============================
    //
    // Return the size-in-words of the
    // agegroup0 buffer for this task:
    //
    return    task->real_heap_allocation_limit
          -   task->heap_allocation_buffer;
}



//////////////////////////////////////////////////////////////
// Hugechunk quires
//
// The hugechunk facility is for values so big that
// it is unreasonable to copy them when cleaning.
//
// Currently, the only hugechunks are code chunks.

#define LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES		10   	// 1KB
#define HUGECHUNK_RAM_QUANTUM_IN_BYTES			(1 << LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES)
#define MINIMUM_HUGECHUNK_QUIRE_BYTESIZE		(128*ONE_K_BINARY)

// A hugechunk "quire" -- a multipage ram region allocated
// directly from the host OS (i.e., not through malloc()):
//
struct hugechunk_quire {
    //
    Punt first_ram_quantum;					// Address of the first ram quantum of the region.
    //
    int	 page_count;						// Number of hugechunk pages in this region.
    int	 free_pages;						// Number of free pages.
    int	 age_of_youngest_live_chunk_in_quire;			// Minimum age over all live hugechunks in region.
    //
    Quire*		quire;					// Quire (multipage ram region) from which we allocate.
    Hugechunk_Quire*	next;					// Next heapchunk_quire in linklist.
    Hugechunk*		hugechunk_page_to_hugechunk[1];		// MUST BE LAST!  Map from hugechunk pages to hugechunks. ('1' is a phony dimension.)
};

// Size of a hugechunk_quire record for
// a quire containing a given number
// of hugechunk pages:
//
#define HUGECHUNK_QUIRE_RECORD_BYTESIZE( NPAGES )    (sizeof(Hugechunk_Quire) + ((NPAGES-1)*sizeof(Hugechunk*)))					// "-1" because struct declaration has   hugechunk_page_to_hugechunk[1]


// Map an address to a hugechunk page index:
//
#define GET_HUGECHUNK_FOR_POINTER_PAGE(hugechunk_quire, address)    (((Punt)(address) - (hugechunk_quire)->first_ram_quantum) >> LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES)


inline Hugechunk*   get_hugechunk_holding_pointee   (Hugechunk_Quire* hq,  Val pointer)   {
    //              =============================
    //
    // Map an address to the corresponding Hugechunk*
    //
    return   hq->hugechunk_page_to_hugechunk[   GET_HUGECHUNK_FOR_POINTER_PAGE( hq, pointer )   ];
}


// Values for below   hugechunk_state   field.
//
// !! Note that even vs odd numbers
//    matter to HUGECHUNK_IS_IN_FROMSPACE
//    (below). 
//
#define   FREE_HUGECHUNK				0	// A free hugechunk.
#define JUNIOR_HUGECHUNK				1	// A junior hugechunk, i.e., one that has never survived a heapcleaning in its current agegroup.
#define JUNIOR_HUGECHUNK_WAITING_TO_BE_FORWARDED	2	// Temporary state during an ongoing heapcleaning: A JUNIOR_HUGECHUNK due to become an SENIOR_HUGECHUNK in the same        agegroup.
#define SENIOR_HUGECHUNK				3	// A senior hugechunk, i.e., one that has survived a heapcleaning in its current agegroup.
#define SENIOR_HUGECHUNK_WAITING_TO_BE_PROMOTED		4	// Temporary state during an ongoing heapcleaning: An SENIOR_HUGECHUNK due to become a JUNIOR_HUGECHUNK in the next-oldest agegroup.

#define HUGECHUNK_IS_IN_FROMSPACE(dp)	(((dp)->hugechunk_state & 0x1) != 0)		// Ickytricky.	Used only once, in   src/c/heapcleaner/heapclean-n-agegroups.c
#define HUGECHUNK_IS_FREE(dp)		((dp)->hugechunk_state == FREE_HUGECHUNK)

//
struct hugechunk {
    //
    Punt	    chunk;			// The actual chunk.

    Punt	    bytesize;			// Size of the chunk in bytes.  When the chunk
						// is in the free list, this will be a multiple of
						// HUGECHUNK_RAM_QUANTUM_IN_BYTES, otherwise it is the exact size.

    unsigned char   huge_ilk;			// The hugechunk sib.  Currently always CODE__HUGE_SIB -- def in   src/c/h/sibid.h
    unsigned char   hugechunk_state;		// The state of the chunk -- see above #defines.
    unsigned char   age;			// The chunk's agegroup.

    Hugechunk_Quire* hugechunk_quire;		// The Hugechunk_Quire holding this hugechunk.

    Hugechunk* prev;				// The prev and next links.  The hugechunk freelist
    Hugechunk* next;				// is a doubly linked list; the other lists are singly linked.
};


inline Punt    uprounded_hugechunk_bytesize   (Hugechunk* hugechunk)   {
    //                      =================================
    //
    // The size of a hugechunk, rounded up to a
    // multiple of our hugechunk ram quantum:
    //
    return   ROUND_UP_TO_POWER_OF_TWO( hugechunk->bytesize, HUGECHUNK_RAM_QUANTUM_IN_BYTES );
}



inline Punt   hugechunk_size_in_hugechunk_ram_quanta   (Hugechunk* hugechunk)   {
    //                     ======================================
    //
    // Number of hugechunk pages occupied by a hugechunk:
    //
    return   uprounded_hugechunk_bytesize( hugechunk )
             >>
             LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES;
}



inline void   remove_hugechunk_from_doubly_linked_list   (Hugechunk* hugechunk)   {
    //        ========================================
    //
    Hugechunk* prev =  hugechunk->prev; 
    Hugechunk* next =  hugechunk->next; 
    //
    prev->next =  next;
    next->prev =  prev;
}


inline void   insert_hugechunk_in_doubly_linked_list   (Hugechunk* header,  Hugechunk* hugechunk)   {
    //        ======================================
    //
    // This is referenced (only) in
    //
    //     src/c/heapcleaner/import-heap.c
    //     src/c/heapcleaner/hugechunk.c
    //
    hugechunk->prev =  header;
    hugechunk->next =  header->next;
    //
    header->next->prev =  hugechunk;
    header->next       =  hugechunk;
}


///////////////////////////////////////////////////////
// Operations on forward pointers
//

// Follow a forward pointer.
// P is the pointer to the chunk.
// NOTE: We need the double type-casts for 32/64 bit systems.
//
#define FOLLOW_FORWARDING_POINTER(chunkheader)		((Val*) (((Val*)(chunkheader))[0]))

// Follow a pair-space forward pointer.
// This is tagged as a descriptor:
//
#define FOLLOW_PAIRSPACE_FORWARDING_POINTER(DESC, chunkheader)	((Val*) (((Punt)(DESC)) & ~ATAG_MASK))		// Clear the 0x2 bit that distinguishes a forwarding pointer and return it.

// Make a pair-space forward pointer.
// This is tagged as a descriptor:
//
#define MAKE_PAIRSPACE_FORWARDING_POINTER(NEW_ADDR)	((Val)((Punt)(NEW_ADDR) | TAGWORD_ATAG))		// TAGWORD_ATAG							def in   src/c/h/heap-tags.h


// Public heapcleaning functions:
//
extern void   heapclean_agegroup0    (Task* task,  Val** roots);						// heapclean_agegroup0						def in   src/c/heapcleaner/heapclean-agegroup0.c
extern void   heapclean_n_agegroups  (Task* task, Val** roots, int level);					// heapclean_n_agegroups					def in   src/c/heapcleaner/heapclean-n-agegroups.c 
//
extern void  zero_agegroup0_overrun_tripwire_buffer( Task* task );						// zero_agegroup0_overrun_tripwire_buffer			def in   src/c/heapcleaner/heap-debug-stuff.c
extern void  check_agegroup0_overrun_tripwire_buffer( Task* task, char* caller );				// check_agegroup0_overrun_tripwire_buffer			def in   src/c/heapcleaner/heap-debug-stuff.c
extern void  dump_all(				Task* task, char* caller );					// dump_all							def in   src/c/heapcleaner/heap-debug-stuff.c
extern void  dump_all_but_hugechunks_contents(	Task* task, char* caller );					// dump_all_but_hugechunks_contents				def in   src/c/heapcleaner/heap-debug-stuff.c
extern void  dump_task(				Task* task, char* caller );					// dump_task							def in   src/c/heapcleaner/heap-debug-stuff.c
extern void  dump_gen0(				Task* task, char* caller );					// dump_gen0							def in   src/c/heapcleaner/heap-debug-stuff.c
extern void  dump_gen0s(			Task* task, char* caller );					// dump_gen0s							def in   src/c/heapcleaner/heap-debug-stuff.c
extern void  dump_gen0_tripwire_buffers(	Task* task, char* caller );					// dump_gen0_tripwire_buffers					def in   src/c/heapcleaner/heap-debug-stuff.c
extern void  dump_gens(				Task* task, char* caller );					// dump_gens							def in   src/c/heapcleaner/heap-debug-stuff.c
extern void  dump_hugechunks_summary(		Task* task, char* caller );					// dump_hugechunks_summary					def in   src/c/heapcleaner/heap-debug-stuff.c
extern void  dump_hugechunks_contents(		Task* task, char* caller );					// dump_hugechunks_contents					def in   src/c/heapcleaner/heap-debug-stuff.c
extern void  dump_ramlog(			Task* task, char* caller );					// dump_ramlog							def in   src/c/heapcleaner/heap-debug-stuff.c
extern void  dump_whatever(			Task* task, char* caller );					// dump_whatever						def in   src/c/heapcleaner/heap-debug-stuff.c
//
extern Status  set_up_tospace_sib_buffers_for_agegroup  (Agegroup* age);					// set_up_tospace_sib_buffers_for_agegroup			def in   src/c/heapcleaner/heapcleaner-stuff.c
extern void    make_new_coarse_inter_agegroup_pointers_map_for_agegroup  (Agegroup* age);			// make_new_coarse_inter_agegroup_pointers_map_for_agegroup	def in   src/c/heapcleaner/heapcleaner-stuff.c
//
extern void    free_agegroup			(Heap* heap, int g);						// free_agegroup						def in   src/c/heapcleaner/heapcleaner-stuff.c
extern void    set_book2sibid_entries_for_range
	           (Sibid* book2sibid,  Val* base,  Vunt bytesize,  Sibid id);				// set_book2sibid_entries_for_range				def in   src/c/heapcleaner/heapcleaner-stuff.c
extern void    null_out_newly_dead_weakrefs	(Heap* heap);							// null_out_newly_dead_weakrefs					def in   src/c/heapcleaner/heapcleaner-stuff.c
//
extern Hugechunk*   allocate_hugechunk_quire (Heap* heap,  Punt bytesize);					// allocate_hugechunk_quire					def in   src/c/heapcleaner/hugechunk.c
extern Hugechunk*   allocate_hugechunk        (Heap* heap,  int gen, Punt chunk_bytesize);			// allocate_hugechunk						def in   src/c/heapcleaner/hugechunk.c
extern void	    free_hugechunk            (Heap* heap,  Hugechunk* chunk);					// free_hugechunk						def in   src/c/heapcleaner/hugechunk.c
extern Hugechunk*   address_to_hugechunk      (Val addr);							// address_to_hugechunk						def in   src/c/heapcleaner/hugechunk.c
//
extern Unt8*        get_codechunk_comment_string_else_null   (Hugechunk* bdp);					// get_codechunk_comment_string_else_null			def in   src/c/heapcleaner/hugechunk.c

#ifdef BO_DEBUG
    extern void print_hugechunk_quire_map (Hugechunk_Quire *r);						// print_hugechunk_quire_map					def in   src/c/heapcleaner/hugechunk.c
#endif

#ifdef CHECK_GC
    extern void check_heap (Heap* heap, int max_swept_age);
#endif

#endif			// HEAP_H


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
# outline-regexp: "[a-z]"			 		 	 #
# End:									 #
##########################################################################
*/
