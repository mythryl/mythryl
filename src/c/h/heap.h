// heap.h
//
// Mythryl-heap-related types, functions, macros and constants.


/*
###              "It is only with the heart one can see clearly;
###               what is essential is invisible to the eye."
###
###                             -- Antoine de Saint-Exupery
*/


// NB: If we need a better name for 'multipage ram region',
// 'quire' (pronounced like choir) deserves consideration:
//     2. A collection of leaves of parchment or paper, folded one within the other, in a manuscript or book.
//      -- http://www.thefreedictionary.com/quire

#ifndef HEAP_H
#define HEAP_H

#include "runtime-base.h"
#include "heapcleaner.h"
#include "sibid.h"
#include "heap-tags.h"

#ifndef OBTAIN_MULTIPAGE_RAM_REGION_FROM_OS_H
    typedef   struct multipage_ram_region
                     Multipage_Ram_Region;
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
    int  oldest_agegroup_keeping_idle_fromspace_buffers;			// We keep (instead of freeing) idle fromspaces for this and all younger agegroups.
};

// Forward declarations to enable mutual recursion:
//
typedef   struct repair            Repair;
typedef   struct sib               Sib;
typedef   struct hugechunk_region  Hugechunk_Region;
typedef   struct hugechunk  	   Hugechunk;
typedef   struct agegroup          Agegroup;

/* typedef   struct heap   Heap; */						// From  src/c/h/runtime-base.h



										// Multipage_Ram_Region		def in    src/c/h/get-multipage-ram-region-from-os.h
										// struct multipage_ram_region	def in    src/c/ram/get-multipage-ram-region-from-mmap.c
										// struct multipage_ram_region	def in    src/c/ram/get-multipage-ram-region-from-mach.c
										// struct multipage_ram_region	def in    src/c/ram/get-multipage-ram-region-from-win32.c


// A heap consists of an agegroup0 and one or more older agegroups.
//
struct heap {
    Val*			agegroup0_buffer;				// Base address of agegroup0 buffer.
    Punt			agegroup0_buffer_bytesize;			// Size-in-bytes of the agegroup0 buffer.
    Multipage_Ram_Region*	multipage_ram_region;				// The memory region we got from the host OS to contain the book_to_sibid__global and agegroup0 buffer.

    int  active_agegroups;							// Number of active agegroups.
    int  oldest_agegroup_keeping_idle_fromspace_buffers;			// Save the from-space for agegroups 1..oldest_agegroup_keeping_idle_fromspace_buffers.
    int  agegroup0_cleanings_done;						// Count how many times we've cleaned (garbage-collected) heap agegroup zero.

    Agegroup*	        agegroup[ MAX_AGEGROUPS ];				// Age-group #i is in agegroup[i-1]
    int		        hugechunk_ramregion_count;				// Number of active hugechunk regions.
    Hugechunk_Region*   hugechunk_ramregions;					// List of hugechunk regions.
    Hugechunk*		hugechunk_freelist;					// Freelist header for hugechunks.

    Val*  weak_pointers_forwarded_during_cleaning;				// List of weak pointers forwarded during cleaning.

    //
    Bigcounter   total_bytes_allocated;						// Cleaner statistics -- tracks number of bytes  allocated.
    Bigcounter   total_bytes_copied_to_sib[ MAX_AGEGROUPS ][ MAX_PLAIN_ILKS ];	// Cleaner statistics -- tracks number of bytes copied into each sib buffer.
};

#ifdef OLD
    // Once we figure out multiple arenas for the multicore version
    // we should be able to go back to the old version of this.		XXX BUGGO FIXME
    //
    #define HEAP_ALLOCATION_LIMIT(hp)			\
	(Val *)((Punt)((hp)->agegroup0_buffer) + (hp)->agegroup0_buffer_bytesize - MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER)
#else
    #define HEAP_ALLOCATION_LIMIT_SIZE(base,size)	\
        (Val*)((Punt)(base) + (size) - MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER)	// "Private" def -- this def is directly referenced only in the next one.

    #define HEAP_ALLOCATION_LIMIT(hp)			\
	HEAP_ALLOCATION_LIMIT_SIZE((hp)->agegroup0_buffer,(hp)->agegroup0_buffer_bytesize)
#endif


// An age-group:
//
struct agegroup {
    //
    Heap*   heap;		// A back pointer to the heap record.
    int	    age;		// Which agegroup this is (1..active_agegroups).
    int	    cleanings;		// Number of times this agegroup has been cleaned.
    int	    ratio;		// Desired number of collections of the previous agegroup for one collection of this agegroup.

    int	    last_cleaning_count_of_younger_agegroup;	// Number cleanings of the previous (younger) agegroup last time this agegroup was cleaned.

    Sib*    sib[ MAX_PLAIN_ILKS ];				// MAX_PLAIN_ILKS		def in    src/c/h/sibid.h

    Hugechunk*   hugechunks[ MAX_HUGE_ILKS ];			// MAX_HUGE_ILKS		def in    src/c/h/sibid.h

    Multipage_Ram_Region*    tospace_ram_region;		// The host-OS multipage ram regions that this agegroup is
    Multipage_Ram_Region*    fromspace_ram_region;		// using for the to-space and from-space.
    Multipage_Ram_Region*    saved_fromspace_ram_region;	// For younger agegroups, we keep the from-space ram region, instead of giving it back.
    //
    Coarse_Inter_Agegroup_Pointers_Map*				// Coarse_Inter_Agegroup_Pointers_Map	def in   src/c/h/coarse-inter-agegroup-pointers-map.h
    coarse_inter_agegroup_pointers_map;				// The dirty cards in the vector sib for this agegroup.
};


// Each heap agegroup has four sib buffers, one each to hold
//
//     pairs
//     records
//     strings
//     vectors 
//
// During garbage collection we will actually have
// two buffers per sib: one from-space, one to-space.
//
struct sib {
    Sibid		id;					// The to-space version of this sib's identifier.
    Val*		next_tospace_word_to_allocate;		// The next word to allocate in this sib's to-space.
    //
    Val*	        tospace;				// Base address and size of to-space.
    Punt		tospace_bytesize;
    Val*	        tospace_limit;				// The top of the to-space (tospace+tospace_bytesize).
    //
    Val*		next_word_to_sweep_in_tospace;		// The next word to sweep in the to-space buffer.
    Repair*		repairlist;				// Points to the top of the repair list (for pickling datastructures).  The repair list grows  down in to-space.

    Val*		fromspace;				// Base address and size of from-space.
    Val_Sized_Unt	fromspace_bytesize;
    Val*		fromspace_used_end;			// The top of the used portion of from-space.

    Val*		end_of_fromspace_oldstuff;		// The top of the "older" from-space region. Chunks below this get promoted, those above don't.
    Sib*		sib_for_promoted_chunks;		// Next older sib, except for oldest sib, which points to itself.

    Bool		heap_needs_repair;			// Set to TRUE when exporting if the sib had
								// external references that require repair.
    // Heap sizing parameters:
    //
    Val_Sized_Unt	requested_sib_buffer_bytesize;	// Requested minimum size for this sib buffer. (This is in addition to the required minimum size.)
    Val_Sized_Unt	soft_max_bytesize;			// A soft maximum size for this sib buffer.
};

//
inline void   make_sib_tospace_into_fromspace   (Sib* sib)   {
    //        ===============================
    //
    // Make to-space into from-space:
    //
    sib->fromspace		=  sib->tospace;
    sib->fromspace_bytesize	=  sib->tospace_bytesize;
    sib->fromspace_used_end	=  sib->next_tospace_word_to_allocate;
}

//
inline Bool   sib_is_active   (Sib* sib)   {
    //        =============
    //
    // Return TRUE iff this sib has an allocated to-space:
    //
    return  sib->tospace_bytesize  >  0;
}

//
inline Punt   sib_freespace_in_bytes   (Sib* sib)   {
    //        ======================
    //
    // Return the amount of free space
    // (in bytes) available in a sib buffer:
    //
    return  (Punt)  sib->tospace_limit
          - (Punt)  sib->next_tospace_word_to_allocate;
}

//
inline Punt   sib_space_used_in_bytes   (Sib* sib)   {
    //        =======================
    //
    // Return the amount of allocated space
    // (in bytes) in a sib buffer:
    //
    return   (Punt)   sib->next_tospace_word_to_allocate
             -
             (Punt)   sib->tospace;
}


// 
inline Bool   sib_chunk_is_old   (Sib* sib,  Val* pointer)   {
    //        ================
    //
    // Return TRUE iff 'pointer' is in
    // the 'old' segment of this sib:
    //
    return   pointer < sib->end_of_fromspace_oldstuff;
}



//////////////////////////////////////////////////////////////
// Hugechunk regions
//
// The hugechunk facility is for values so big that
// it is unreasonable to copy them when cleaning.
//
// Currently, the only hugechunks are code chunks.

#define LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES		10   	// 1KB
#define HUGECHUNK_RAM_QUANTUM_IN_BYTES			(1 << LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES)
#define MINIMUM_HUGECHUNK_RAM_REGION_BYTESIZE	(128*ONE_K_BINARY)

// A hugechunk region:
//
struct hugechunk_region {
    //
    Punt first_ram_quantum;			// Address of the first ram quantum of the region.
    //
    int	page_count;						// Number of hugechunk pages in this region.
    int	free_pages;						// Number of free pages.
    int	age_of_youngest_live_chunk_in_region;			// Minimum age over all live hugechunks in region.
    //
    Multipage_Ram_Region*  ram_region;				// Ram region from which we allocate.
    Hugechunk_Region*      next;				// Next region in the list of regions.
    Hugechunk*		   hugechunk_page_to_hugechunk[1];	// MUST BE LAST!  Map from hugechunk pages to hugechunks. ('1' is a phony dimension.)
};

// Size of a hugechunk region record for
// a region containing a given number
// of hugechunk pages:
//
#define HUGECHUNK_REGION_RECORD_BYTESIZE(NPAGES)    (sizeof(Hugechunk_Region) + ((NPAGES-1)*sizeof(Hugechunk*)))					// "-1" because struct declaration has   hugechunk_page_to_hugechunk[1]


// Map an address to a hugechunk page index:
//
#define GET_HUGECHUNK_FOR_POINTER_PAGE(region, address)    (((Punt)(address) - (region)->first_ram_quantum) >> LOG2_HUGECHUNK_RAM_QUANTUM_IN_BYTES)


inline Hugechunk*   get_hugechunk_holding_pointee   (Hugechunk_Region* region,  Val pointer)   {
    //              =============================
    //
    // Map an address to the corresponding Hugechunk*
    //
    return   region->hugechunk_page_to_hugechunk[   GET_HUGECHUNK_FOR_POINTER_PAGE( region, pointer )   ];
}


// Values for below   hugechunk_state   field.
//
// !! Note that even vs odd numbers
//    matter to HUGECHUNK_IS_IN_FROMSPACE
//    (below). 
//
#define            FREE_HUGECHUNK	0	// A free hugechunk.
#define           YOUNG_HUGECHUNK	1	// A young hugechunk, i.e., one that has never been forwarded in its agegroup.
#define YOUNG_FORWARDED_HUGECHUNK	2	// A forwarded young hugechunk.
#define             OLD_HUGECHUNK	3	// An old hugechunk.
#define    OLD_PROMOTED_HUGECHUNK	4	// A promoted old hugechunk.

#define HUGECHUNK_IS_IN_FROMSPACE(dp)	(((dp)->hugechunk_state & 0x1) != 0)		// Ickytricky.
#define HUGECHUNK_IS_FREE(dp)		((dp)->hugechunk_state == FREE_HUGECHUNK)

//
struct hugechunk {
    //
    Punt	    chunk;			// The actual chunk.

    Punt	    bytesize;		// Size of the chunk in bytes.  When the chunk
						// is in the free list, this will be a multiple of
						// HUGECHUNK_RAM_QUANTUM_IN_BYTES, otherwise it is the exact size.

// RENAME THIS TO huge_ilk!! XXX BUGGO FIXME
    unsigned char   huge_ilk;			// The chunk ilk.  Currently always CODE__HUGE_ILK -- def in   src/c/h/sibid.h
    unsigned char   hugechunk_state;		// The state of the chunk -- see above #defines.
    unsigned char   age;			// The chunk's agegroup.

    Hugechunk_Region* region;			// The region this big chunk is in.

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
#define FOLLOW_FWDCHUNK(chunkheader)		((Val*) (((Val*)(chunkheader))[0]))

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
extern void  zero_out_agegroup0_overrun_tripwire_buffer( Task* task );						// zero_out_agegroup0_overrun_tripwire_buffer			def in   src/c/heapcleaner/heapcleaner-stuff.c
extern void  validate_agegroup0_overrun_tripwire_buffer( Task* task, char* caller );				// validate_agegroup0_overrun_tripwire_buffer			def in   src/c/heapcleaner/heapcleaner-stuff.c
//
extern Status  allocate_and_partition_an_agegroup  (Agegroup* age);						// allocate_and_partition_an_agegroup				def in   src/c/heapcleaner/heapcleaner-stuff.c
extern void    make_new_coarse_inter_agegroup_pointers_map_for_agegroup  (Agegroup* age);			// make_new_coarse_inter_agegroup_pointers_map_for_agegroup	def in   src/c/heapcleaner/heapcleaner-stuff.c
//
extern void    free_agegroup			(Heap* heap, int g);						// free_agegroup						def in   src/c/heapcleaner/heapcleaner-stuff.c
extern void    set_book2sibid_entries_for_range
	           (Sibid* book2sibid,  Val* base,  Val_Sized_Unt bytesize,  Sibid id);				// set_book2sibid_entries_for_range				def in   src/c/heapcleaner/heapcleaner-stuff.c
extern void    null_out_newly_dead_weak_pointers	(Heap* heap);						// null_out_newly_dead_weak_pointers				def in   src/c/heapcleaner/heapcleaner-stuff.c
//
extern Hugechunk*   allocate_hugechunk_region (Heap* heap,  Punt bytesize);					// allocate_hugechunk_region					def in   src/c/heapcleaner/hugechunk.c
extern Hugechunk*   allocate_hugechunk        (Heap* heap,  int gen, Punt chunk_bytesize);			// allocate_hugechunk						def in   src/c/heapcleaner/hugechunk.c
extern void	    free_hugechunk            (Heap* heap,  Hugechunk* chunk);					// free_hugechunk						def in   src/c/heapcleaner/hugechunk.c
extern Hugechunk*   address_to_hugechunk      (Val addr);							// address_to_hugechunk						def in   src/c/heapcleaner/hugechunk.c
//
extern Unt8*        get_codechunk_comment_string_else_null   (Hugechunk* bdp);					// get_codechunk_comment_string_else_null			def in   src/c/heapcleaner/hugechunk.c

#ifdef BO_DEBUG
    extern void print_hugechunk_region_map (Hugechunk_Region *r);						// print_hugechunk_region_map					def in   src/c/heapcleaner/hugechunk.c
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
