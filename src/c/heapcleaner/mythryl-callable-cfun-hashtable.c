// mythryl-callable-cfun-hashtable.c
//
//
//
// Problem
// =======
//
// When we write a heap image to disk, pointers from the
// heap to C and assembly-language functions (etc) must
// be handled specially, since they are not actually
// included in the heap image, and may be be at different
// addresses when the heap image is later loaded (if the
// runtime code has been modified in the meantime).
//
//
//
// Solution
// ========
//
// We include in the heap image a "cfun table" of all
// such references, by name, and then in the disk image
// replace those referenced by specially marked indices
// into the cfun table.
//
// Later, when we load the heap image in, we will look up
// the "cfun table" symbols by name and patch the image
// appropriately so those references point to the same
// functions as before -- at possibly different addresses.

#include "../config.h"

#include <string.h>

#include "runtime-base.h"
#include "heap-tags.h"
#include "runtime-values.h"
#include "mythryl-callable-cfun-hashtable.h"

													// MAKE_TAGWORD						def in    src/c/h/heap-tags.h
#define MAKE_EXTERN(index)	MAKE_TAGWORD(index, EXTERNAL_REFERENCE_IN_EXPORTED_HEAP_IMAGE_BTAG)	// EXTERNAL_REFERENCE_IN_EXPORTED_HEAP_IMAGE_BTAG	def in    

#define HASH_STRING( name, result )	{			\
	const char	*__cp = (name);				\
	int	__hash = 0, __n;				\
	for (;  *__cp;  __cp++) {				\
	    __n = (128*__hash) + (unsigned)*__cp;		\
	    __hash = __n - (8388593 * (__n / 8388593));		\
	}							\
	(result) = __hash;					\
    }

// We use the usual hashtable scheme of a vector
// of buckets each holding a linklist of items.
//
// Since we want to be able to retrieve both by name
// and by address, we actually maintain two hashtable
// bucket-vectors, one hashed by name, one hashed by
// address.

// Since every item will be in both tables, instead of
// creating two separate linklist records for each item
// (one per table), we use a single record which gets
// entered into both tables.  To make this work, each
// record has two 'next' pointers, one for each bucketlist.
//

//
typedef struct hashtable_entry {				// An item in the Symbol/Addr tables.
    //
    Val addr;							// The address of the external reference.
    //
    const char*  name;		    				// The name of the reference.
    const char*  nickname;		  			// The nickname of the reference.
    //
    int	       hash_of_name;	    				// The hash sum of the name.
    int	       hash_of_nickname;	  			// The hash sum of the name.
    //
    struct hashtable_entry* next_by_name_table_item; 		// The next item in the hashed-by-name     bucketlist.
    struct hashtable_entry* next_by_nickname_table_item; 	// The next item in the hashed-by-nickname bucketlist.
    struct hashtable_entry* next_by_address_table_item;		// The next item in the hashed-by-address  bucketlist.
    //
} Hashtable_Entry;
    //
    // 2010-12-15 CrT: All this 'nickname' stuff is temporary
    // scaffolding to support renaming.  I've been working
    // my way through the cfun-list.h files giving things
    // decent names, and without some facility such as above
    // 'nickname', changing the names shared between C and
    // Mythryl is a pain because they are cached in the heapfiles,
    // in particular bin/mythryld.  The 'nickname' facility allows
    // two names to be registered for one entry, allowing an
    // inchworm-style three-phase renaming protocol:
    //
    //     1) Change main name in cfun-list.h CFUNC() entry to
    //        new value, and do a complete compile cycle.  This
    //        establishes the new name in the runtime.
    //
    //     2) Change all mentions in *.pkg from the old name to
    //        the new name and do a complete compile cycle. This
    //        removes all dependencies on the original name.
    //
    //     3) Change nickname in cfun-list.h CFUNC() entry to match
    //        the new name, and do a full compile cycle. Mission complete!
    //
    // Anyhow, this nickname crud should be deleted as soon as all
    // cfun-list.h files have been rationalized.  (It really isn't
    // needed for new libraries because they can be done right the
    // first time, or failing that, the compiler is not yet so dependent
    // upon them that they cannot simply be removed, changed and re-inserted.)
    //
    // I'm suspending work on this front for the moment because it
    // is great rainy-day/intersticial-time work but a waste of
    // prime programming time;  Right now I want to focuse on the
    // transition to 64-bit code -- the Last Scary Mythryl Hack.

typedef struct hashtable_entry_ref {		// An item in an cfun table.
    //
    Hashtable_Entry* item;
    int   index;
    //
    struct hashtable_entry_ref*  next;
    //
} Heapfile_Cfun_Table_Entry;

// A table of C symbols mapping strings to items,
// which is used when loading a heap image:
//
struct heapfile_cfun_table {						// We do   'typedef struct heapfile_cfun_table Heapfile_Cfun_Table;'	in   src/c/h/mythryl-callable-cfun-hashtable.h
    //
    Heapfile_Cfun_Table_Entry**   bucket_vector;
    int		         bucketvector_size_in_buckets;
    //
    int  entries_count;						// Number of actual entries in table.
    //
    Hashtable_Entry** all_entries;				// Contains all hashtable entries in allocation order.
    int		      all_entries_size_in_slots;		// Physical size of previous.
};		

// Hash key to bucket-vector index.
//
// We assume 'bucketvector_size_in_slots' is a power of two, so 'bucketvector_size_in_slots-1'
// will be a bitmask like 0xFFF good for cutting
// the hashword down to size of bucket-vector.
//
#define COMPUTE_BUCKETVECTOR_INDEX_FROM_NAME_HASH(h, bucketvector_size_in_slots)	((h)                       & ((bucketvector_size_in_slots)-1))
#define COMPUTE_BUCKETVECTOR_INDEX_FROM_ADDRESS(  a, bucketvector_size_in_slots)	(((Val_Sized_Unt)(a) >> 3) & ((bucketvector_size_in_slots)-1))		// '>> 3' to drop uninteresting lower bits.

// The following five variables hold the
// state of our anonymous hashtable.  Client
// access to this table is via:
//     publish_cfun()
//     name_of_cfun()
//     find_cfun()
//
static Hashtable_Entry**	hashed_by_name_bucketvector_local     =  NULL;	// Holds the      hashtable contents hashed by name.
static Hashtable_Entry**	hashed_by_nickname_bucketvector_local =  NULL;	// Holds the      hashtable contents hashed by nickname.
static Hashtable_Entry**	hashed_by_address_bucketvector_local  =  NULL;	// Holds the same hashtable contents hashed by address.
//
static int	                bucketvector_size_in_slots_local      =  0;	// Number of slots in the above two.  Must be a power of two!
static int	                hashtable_entries_count_local  	      =  0;	// Number of entries actually currently stored.

// Local routines:
//
static void   double_size_of_heapfile_cfun_table   (Heapfile_Cfun_Table* table);


void   publish_cfun2   (const char* name,  const char* nickname,  Val addr)   {
    // =============
    //
    // Publish the address of a runtime resource
    // (typically a C-coded function) for use at
    // the Mythryl level.  Later,
    //
    //     heapio__read_externs_table()
    //
    // in  src/c/heapcleaner/import-heap-stuff.c
    //
    // will look these up by name and patch them
    // into a heap being loaded.
    //
    // We get called from:
    //     src/c/main/construct-runtime-package.c				// To publish the contents of the bootstrap   runtime   package defined in    src/lib/core/init/runtime.pkg
    //     src/c/lib/mythryl-callable-c-libraries.c			// To publish all the     src/c/lib/*/*.c     fns.

    int		n;
    int		hash;
    int		nickhash;

    Hashtable_Entry*	item;

    ASSERT ((((Val_Sized_Unt)addr & ~POINTER_ATAG) & TAGWORD_ATAG) == 0);

    // To keep lookup fast, we store only as many
    // items in the hashtable as there are buckets.
    // This means the average bucket linklist length
    // is at most one:
    //
    if (hashtable_entries_count_local == bucketvector_size_in_slots_local) {
        //
        // Hashtable is now "full" (as many items as buckets)
        // so we double its size:

	// Pick new bucket count.  We start with 64 buckets,
	// then successively double as needed:
	//
	int  new_bucketvector_size_in_slots
	  =
          bucketvector_size_in_slots_local  ?   2 * bucketvector_size_in_slots_local
                                            :  64;

	// Allocate and zero out our new bucketvectors:
	//
	Hashtable_Entry**  new_hashed_by_name_bucketvector     =  MALLOC_VEC( Hashtable_Entry*, new_bucketvector_size_in_slots );
	Hashtable_Entry**  new_hashed_by_nickname_bucketvector =  MALLOC_VEC( Hashtable_Entry*, new_bucketvector_size_in_slots );
	Hashtable_Entry**  new_hashed_by_address_bucketvector  =  MALLOC_VEC( Hashtable_Entry*, new_bucketvector_size_in_slots );
	//
	memset ((char*) new_hashed_by_name_bucketvector,     0, sizeof(Hashtable_Entry*) * new_bucketvector_size_in_slots );
	memset ((char*) new_hashed_by_nickname_bucketvector, 0, sizeof(Hashtable_Entry*) * new_bucketvector_size_in_slots );
	memset ((char*) new_hashed_by_address_bucketvector,  0, sizeof(Hashtable_Entry*) * new_bucketvector_size_in_slots );

	// Move all existing hashtable entries
	// from old to new bucketvectors:
	//
	for (int i = 0;  i < bucketvector_size_in_slots_local;  i++) {
	    //
	    for (Hashtable_Entry* p = hashed_by_name_bucketvector_local[i];  p != NULL; ) {
	        //
		item = p;
		p = p->next_by_name_table_item;
		n = COMPUTE_BUCKETVECTOR_INDEX_FROM_NAME_HASH(item->hash_of_name, new_bucketvector_size_in_slots);
		item->next_by_name_table_item = new_hashed_by_name_bucketvector[n];
		new_hashed_by_name_bucketvector[n] = item;
	    }

	    if (nickname) {
		for (Hashtable_Entry* p = hashed_by_nickname_bucketvector_local[i];  p != NULL; ) {
		    //
		    item = p;
		    p = p->next_by_nickname_table_item;
		    n = COMPUTE_BUCKETVECTOR_INDEX_FROM_NAME_HASH(item->hash_of_nickname, new_bucketvector_size_in_slots);
		    item->next_by_nickname_table_item = new_hashed_by_nickname_bucketvector[n];
		    new_hashed_by_nickname_bucketvector[n] = item;
		}
	    }

	    for (Hashtable_Entry* p = hashed_by_address_bucketvector_local[i];  p != NULL; ) {
	        //
		item = p;
		p = p->next_by_address_table_item;
		n = COMPUTE_BUCKETVECTOR_INDEX_FROM_ADDRESS(item->addr, new_bucketvector_size_in_slots);
		item->next_by_address_table_item = new_hashed_by_address_bucketvector[n];
		new_hashed_by_address_bucketvector[n] = item;
	    }
	}

	// Recycle the old bucketvectors, if any:
	//
	if (hashed_by_name_bucketvector_local != NULL) {
	    //
	    FREE( hashed_by_name_bucketvector_local     );
	    FREE( hashed_by_nickname_bucketvector_local );
	    FREE( hashed_by_address_bucketvector_local  );
	}

	hashed_by_name_bucketvector_local     =  new_hashed_by_name_bucketvector;
	hashed_by_nickname_bucketvector_local =  new_hashed_by_nickname_bucketvector;
	hashed_by_address_bucketvector_local  =  new_hashed_by_address_bucketvector;
	bucketvector_size_in_slots_local      =  new_bucketvector_size_in_slots;
    }

    // Compute the string hash function:
    //
    HASH_STRING(name, hash);

    if (nickname)  HASH_STRING(nickname, nickhash)
    else                                 nickhash = 0;

    // Allocate the item:
    //
    item = MALLOC_CHUNK( Hashtable_Entry );
    item->name		   = name;
    item->nickname	   = nickname;
    item->hash_of_name	   = hash;
    item->hash_of_nickname = nickhash;
    item->addr		   = addr;

    // Insert the item into the symbol table:
    //
    n = COMPUTE_BUCKETVECTOR_INDEX_FROM_NAME_HASH(hash, bucketvector_size_in_slots_local);
    //
    for (Hashtable_Entry* p = hashed_by_name_bucketvector_local[n];  p != NULL;  p = p->next_by_name_table_item) {
        //
	if (p->hash_of_name == hash
        &&  strcmp( name, p->name ) == 0
        ){
	    if (p->addr != addr)   die( "global C symbol \"%s\" defined twice", name );
	    //
	    FREE( item );
	    return;
	}
    }
    item->next_by_name_table_item	 = hashed_by_name_bucketvector_local[n];
    hashed_by_name_bucketvector_local[n] = item;

    if (nickname) {
	n = COMPUTE_BUCKETVECTOR_INDEX_FROM_NAME_HASH(nickhash, bucketvector_size_in_slots_local);
	//
	for (Hashtable_Entry* p = hashed_by_nickname_bucketvector_local[n];  p != NULL;  p = p->next_by_nickname_table_item) {
	    //
	    if (p->hash_of_nickname == nickhash
	    &&  strcmp( nickname, p->nickname ) == 0
	    ){
		if (p->addr != addr)   die( "global C symbol nickname \"%s\" defined twice", name );
		//
		FREE( item );
		return;
	    }
	}
	item->next_by_nickname_table_item	 = hashed_by_nickname_bucketvector_local[n];
	hashed_by_nickname_bucketvector_local[n] = item;
    }

    // Insert the item into the addr table:
    //
    n = COMPUTE_BUCKETVECTOR_INDEX_FROM_ADDRESS(addr, bucketvector_size_in_slots_local);
    //
    for (Hashtable_Entry* p = hashed_by_address_bucketvector_local[n];  p != NULL;  p = p->next_by_address_table_item) {
        //
	if (p->addr == addr) {
	    //
	    if (p->hash_of_name != hash
            || strcmp(name, p->name) != 0
            ){
		die ("address %#x defined twice: \"%s\" and \"%s\"", addr, p->name, name);
	    }

	    FREE( item );
	    return;
	}
    }

    item->next_by_address_table_item
	=
	hashed_by_address_bucketvector_local[ n ];

    hashed_by_address_bucketvector_local[ n ]
	=
	item;

    ++hashtable_entries_count_local;
}								// fun publish_cfun


void   publish_cfun   (const char* name,  Val addr)   {
    // ============

    publish_cfun2  (name,  NULL, addr);
}

const char*   name_of_cfun   (Val address) {
    //        ============
    //
    // Return the name of the C symbol that
    // labels the given address, else NULL.
    //	
    // This is called (only) from:
    //     src/c/heapcleaner/check-heap.c

    // Find the symbol in the hashed_by_address_bucketvector_local:
    //
    for (Hashtable_Entry*
         q  =  hashed_by_address_bucketvector_local[ COMPUTE_BUCKETVECTOR_INDEX_FROM_ADDRESS( address, bucketvector_size_in_slots_local) ];
	 q !=  NULL;
	 q  =  q->next_by_address_table_item
    ){
	if (q->addr == address)   return q->name;
    }

    return NULL;
}


Val   find_cfun   (const char* name)   {
    //=========
    //
    // We get called from exactly one place,
    // to read in a cfun table in
    //
    //     src/c/heapcleaner/import-heap-stuff.c

    int		       hash;
    HASH_STRING( name, hash );

    int index =  COMPUTE_BUCKETVECTOR_INDEX_FROM_NAME_HASH( hash, bucketvector_size_in_slots_local );

    for (Hashtable_Entry*
        p  = hashed_by_name_bucketvector_local[ index ];
	p != NULL;
	p  = p->next_by_name_table_item
    ){
	if (p->hash_of_name == hash
            &&
            !strcmp( p->name, name )
        ){
	    return  p->addr;
	}
    }

for (Hashtable_Entry*
    p  = hashed_by_nickname_bucketvector_local[ index ];
    p != NULL;
    p  = p->next_by_nickname_table_item
){
    if (p->hash_of_nickname == hash
	&&
	!strcmp( p->nickname, name  )
    ){
	return  p->addr;
    }
}

    return HEAP_VOID;
}


//

Heapfile_Cfun_Table*   make_heapfile_cfun_table   ()   {
    //        =================
    //
    // This function is called from:
    //     src/c/heapcleaner/datastructure-pickler-cleaner.c
    //     src/c/heapcleaner/export-heap.c

    Heapfile_Cfun_Table* table =   MALLOC_CHUNK(Heapfile_Cfun_Table);
    //
    table->bucketvector_size_in_buckets	= 0;
    //
    table->bucket_vector = NULL;
    table->entries_count = 0;
    table->all_entries	 = NULL;
    table->all_entries_size_in_slots	 = 0;
    //
    return table;
}



Val   add_cfun_to_heapfile_cfun_table   (Heapfile_Cfun_Table* table,   Val addr)   {
    //===============================
    //
    // Add an external reference to an export table,
    // returning its coded export-table index.
    //
    // We get called two places each in:
    //
    //     src/c/heapcleaner/export-heap.c
    //     src/c/heapcleaner/datastructure-pickler-cleaner.c

    Punt a =   HEAP_POINTER_AS_UNT( addr );


    // debug_say("add_cfun_to_heapfile_cfun_table: addr = %#x, ", addr);

    if (table->entries_count >= table->bucketvector_size_in_buckets) {
	//
	double_size_of_heapfile_cfun_table( table );
    }

    // If addr is already in table,
    // just return the coded table offset:
    //
    int hash = COMPUTE_BUCKETVECTOR_INDEX_FROM_ADDRESS( a, table->bucketvector_size_in_buckets );
    //
    for (Heapfile_Cfun_Table_Entry*
         p = table->bucket_vector[ hash ];
         p != NULL;
         p = p->next
    ){
	//
	if (p->item->addr == addr) {
	    //
	    // debug_say("old name = \"%s\", index = %d\n", p->item->name, p->index);
	    //
	  return MAKE_EXTERN( p->index );							// MAKE_EXTERN		def above.
	}
    }



    // Ok, we need to create an export table entry
    // for this external reference.

    // Find the referenced external symbol in our
    // hashed_by_address_bucketvector_local:
    //
    Hashtable_Entry* q;
    for (q  = hashed_by_address_bucketvector_local[ COMPUTE_BUCKETVECTOR_INDEX_FROM_ADDRESS( a, bucketvector_size_in_slots_local ) ];
         q != NULL;
         q  = q->next_by_address_table_item
    ){
	if (q->addr == addr)   break;
    }

    if (q == NULL) {
        //
	say_error( "external address %#x not registered\n", addr );
	return HEAP_VOID;
    }

    // Make sure there is room in the
    // export table for our new entry:
    //
    // debug_say("new name = \"%s\", index = %d\n", q->name, table->entries_count);
    //
    int index = table->entries_count++;
    //
    if (table->all_entries_size_in_slots <= index) {
        //
        // The export table is full, so double its size:
        //
	int new_size
	    =
	    table->all_entries_size_in_slots == 0
                ? 64
                : 2 * table->all_entries_size_in_slots;

	Hashtable_Entry** new_map = MALLOC_VEC( Hashtable_Entry*, new_size );

	for (int i = 0;  i < table->all_entries_size_in_slots;  i++) {
	    //
	    new_map[i] = table->all_entries[i];
        }

	if (table->all_entries != NULL)   FREE (table->all_entries);

	table->all_entries = new_map;
	table->all_entries_size_in_slots = new_size;
    }
    table->all_entries[index]	= q;

    // Insert the address into the export table:
    //
    Heapfile_Cfun_Table_Entry*
    p	     =  MALLOC_CHUNK( Heapfile_Cfun_Table_Entry );
    p->item  =  q;
    p->index =  index;
    p->next  =  table->bucket_vector[ hash ];
    //
    table->bucket_vector[ hash ] =  p;


    // Return the coded export-table index
    // which is to replace the original external
    // reference in the on-disk heap image:
    //
    return MAKE_EXTERN( index );
}							// fun add_cfun_to_heapfile_cfun_table



Val   get_cfun_address_from_heapfile_cfun_table   (Heapfile_Cfun_Table* table,  Val xref)   {
    //=========================================
    //
    // Given an external reference, return its address.

    int	index = GET_LENGTH_IN_WORDS_FROM_TAGWORD(xref);

//  debug_say("get_cfun_address_from_heapfile_cfun_table: %#x: %d --> %#x\n", xref, index, table->all_entries[index]->addr);

    if (index >= table->entries_count)	die ("export-table index %d > table size %d?!", index, table->entries_count);

    return table->all_entries[index]->addr;
}


void   get_names_of_all_cfuns_in_heapfile_cfun_table   (Heapfile_Cfun_Table* table,  int* symbol_count,  const char*** symbols)   {
    // =============================================
    //
    // This function is called (only) from:
    //     src/c/heapcleaner/export-heap-stuff.c

    int          n  =  *symbol_count =  table->entries_count;
    const char** ep =  *symbols      =  MALLOC_VEC( const char*, n );

    Hashtable_Entry** p = table->all_entries;

    for (int i = 0;   i < n;   i++, p++, ep++) {
	//
	*ep = (*p)->name;
    }
}


void   free_heapfile_cfun_table   (Heapfile_Cfun_Table* table)   {
    // ========================
    //
    // Free the storage used by an import/export table.


    // Free all the buckets on all the bucket-chains:
    //
    for (int i = 0;   i <  table->bucketvector_size_in_buckets;   i++) {
	//
        Heapfile_Cfun_Table_Entry*  next;
	//
	for (Heapfile_Cfun_Table_Entry*
            p  = table->bucket_vector[ i ];
            p != NULL;
	    p  = next
	){			next =  p->next;
	    FREE( p );
	}
    }

    if (table->all_entries != NULL)   FREE( table->all_entries );

    FREE( table );
}


Punt   heapfile_cfun_table_bytesize   (Heapfile_Cfun_Table* table)   {
    // =================================
    //
    // Return the number of bytes needed to represent
    // the strings in an exported symbols table.

    Punt bytesize =  0;

    for (int i = 0;   i < table->entries_count;   i++) {
	//
	bytesize +=  1 + strlen( table->all_entries[i]->name );
    }

    return  ROUND_UP_TO_POWER_OF_TWO( bytesize, WORD_BYTESIZE );
}


static void   double_size_of_heapfile_cfun_table   (Heapfile_Cfun_Table* table)   {
    //        ==================================
    //


    // Select new hashtable size -- 32 initially,
    // then double each time:
    //	
    int	new_bucketvector_size_in_buckets
	=
	table->bucketvector_size_in_buckets
          ? 2 * table->bucketvector_size_in_buckets
          : 32;


    // Allocate new bucket vector and zero it out:
    //
    Heapfile_Cfun_Table_Entry**  new_bucket_vector
	=
	MALLOC_VEC( Heapfile_Cfun_Table_Entry*, new_bucketvector_size_in_buckets );
    //
    memset( (char*) new_bucket_vector, 0, new_bucketvector_size_in_buckets * sizeof( Heapfile_Cfun_Table_Entry* ) );


    // Move all items from old to new bucket-vector:
    //
    for (int i = 0;   i < table->bucketvector_size_in_buckets;   i++) {
	//
	for(Heapfile_Cfun_Table_Entry* p = table->bucket_vector[i];  p != NULL; ) {
	    Heapfile_Cfun_Table_Entry* q = p;

	    p = p->next;

	    int n = COMPUTE_BUCKETVECTOR_INDEX_FROM_ADDRESS( q->item->addr, new_bucketvector_size_in_buckets );

	    q->next = new_bucket_vector[ n ];

	    new_bucket_vector[ n ] = q;
	}
    }

    if (table->bucket_vector)   FREE( table->bucket_vector );

    table->bucket_vector                =  new_bucket_vector;
    table->bucketvector_size_in_buckets =  new_bucketvector_size_in_buckets;
}



// COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
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

