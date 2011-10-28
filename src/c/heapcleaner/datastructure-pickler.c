// datastructure-pickler.c

#include "../config.h"

#include "system-dependent-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-heap-image.h"
#include "mythryl-callable-cfun-hashtable.h"
#include "address-hashtable.h"
#include "cleaner.h"
#include "datastructure-pickler.h"
#include "export-heap-stuff.h"
#include "heap-io.h"



// For background see:  src/A.DATASTRUCTURE-PICKLING.OVERVIEW



/*
###               "I am one of the culprits who created the problem.
###                I used to write those programs back in the '60s
###                and '70s, and was so proud of the fact that I was
###                able to squeeze a few elements of space by not
###                having to put '19' before the year."
###
###                                         -- Alan Greenspan
*/


#define PICKLER_ERROR	HEAP_VOID

 static Val   pickle_unboxed_value           (Task* task,  Val chunk);
 static Val   pickle_heap_datastructure              (Task* task,  Val chunk,  Pickler_Result* info);
 static Val   allocate_heap_ram_for_pickle  (Task* task,  Punt bytesize);


Val   pickle_datastructure   (Task* task,  Val root_chunk)  {
    //=======================
    //
    // Linearize a Mythryl heap chunk
    // into a vector of bytes.
    // Return HEAP_VOID on errors.
    //
    // This fn gets exported to the Mythryl level as 'pickle_datastructure' via
    //
    //     src/c/lib/heap/datastructure-pickler.c
    //
    // and then
    //
    //     src/lib/std/src/unsafe/unsafe.pkg

    clean_heap_with_extra_roots (task, 0, &root_chunk, NULL);  				// Clean agegroup0.

    int age =  get_chunk_age( root_chunk );						// get_chunk_age			def in   src/c/heapcleaner/get-chunk-age.c

    if (age == -1)   return pickle_unboxed_value( task, root_chunk );

    // A regular Mythryl heap chunk.
    // Do the pickler cleaning:

    // DEBUG  check_heap (task->heap, task->heap->active_agegroups);

    Pickler_Result  pickler_result								// Pickler_Result				def in    src/c/heapcleaner/datastructure-pickler.h
	=
	pickler__clean_heap( task, &root_chunk, age );					// pickler__clean_heap			def in    src/c/heapcleaner/datastructure-pickler-cleaner.c

    Val pickle
	=
	pickle_heap_datastructure( task, root_chunk, &pickler_result );			// Defined below.

    // Repair the heap or finish the cleaning:
    //
    pickler__wrap_up( task, &pickler_result );					// pickler__wrap_up		def in    src/c/heapcleaner/datastructure-pickler-cleaner.c

    // DEBUG check_heap (task->heap, result.maxGen);

    return pickle;
}


static Val   pickle_unboxed_value   (Task* task,  Val root_chunk) {
    //       =======================
    //
    Pickle_Header  pickle_header;
    //
    Val	     pickle;
    Writer*  wr;
    //
    int bytesize =  sizeof( Heapfile_Header )
                      +  sizeof( Pickle_Header );

    // Allocate space for the chunk:
    //
    pickle =  allocate_heap_ram_for_pickle( task, bytesize );
    //
    wr =   WR_OpenMem(   PTR_CAST( Unt8*, pickle ),   bytesize );

    heapio__write_image_header( wr, UNBOXED_PICKLE );								// heapio__write_image_header		def in    src/c/heapcleaner/export-heap-stuff.c

    pickle_header.smallchunk_sibs_count     =  0;
    pickle_header.hugechunk_sibs_count      =  0;
    pickle_header.hugechunk_ramregion_count =  0;
    //
    pickle_header.contains_code	           =  FALSE;
    pickle_header.root_chunk		   =  root_chunk;

    WR_WRITE( wr, &pickle_header, sizeof( pickle_header ) );

    if (WR_ERROR(wr)) 	return HEAP_VOID;

    WR_FREE(wr);

    SEQHDR_ALLOC( task, pickle, STRING_TAGWORD, pickle, bytesize );

    return pickle;
}


static Val   pickle_heap_datastructure   (Task *task,  Val root_chunk,  Pickler_Result* result)   {
    //       ====================
    //
    Heap* heap    =  task->heap;
    int	  max_age =  result->oldest_agegroup_included_in_pickle;

    Punt  total_sib_buffer_bytesize[ MAX_PLAIN_ILKS ];
    Punt  total_bytesize;

    struct {
	Punt		    base;	// Base address of the sib buffer in the heap.
	Punt		    offset;	// Relative position in the merged sib buffer.
	//
    } adjust[ MAX_AGEGROUPS ][ MAX_PLAIN_ILKS ];

    Sib_Header*  p;										// Sib_Header		def in    src/c/heapcleaner/runtime-heap-image.h
    Sib_Header*  sib_headers[ TOTAL_ILKS ];
    Sib_Header*  sib_header_buffer;

    int  sib_header_bytesize;
    int	 smallchunk_sibs_count;

    Val     pickle;
    Writer* wr;

    // Compute the sib offsets in the heap image:
    //
    for (int ilk = 0;   ilk < MAX_PLAIN_ILKS;   ilk++) {
        //
	total_sib_buffer_bytesize[ ilk ] = 0;
    }

    // The embedded literals go first:
    //
    total_sib_buffer_bytesize[ STRING_ILK ]						// pickler__relocate_embedded_literals	def in   src/c/heapcleaner/datastructure-pickler-cleaner.c
	=
	pickler__relocate_embedded_literals( result, STRING_ILK, 0 );

    // DEBUG debug_say("%d bytes of string literals\n", total_sib_buffer_bytesize[STRING_ILK]);

    for     (int age = 0;  age < max_age;         age++) {
	for (int ilk = 0;  ilk < MAX_PLAIN_ILKS;  ilk++) {
	    //
	    Sib* sib =  heap->agegroup[ age ]->sib[ ilk ];

	    adjust[ age ][ ilk ].offset
		=
		total_sib_buffer_bytesize[ ilk ];

	    if (!sib_is_active(sib)) {								// sib_is_active	def in    src/c/h/heap.h
	        //
		adjust[ age ][ ilk ].base =  0;
		//
	    } else {
		//
		total_sib_buffer_bytesize[ ilk ]
		   +=
		    (Punt)  sib->next_tospace_word_to_allocate
		    -
		    (Punt)  sib->tospace;

		adjust[ age ][ ilk ].base =  (Punt) sib->tospace;
	    }
	}
    }

    // DEBUG for (ilk = 0;  ilk < MAX_PLAIN_ILKS;  ilk++) debug_say ("sib %d: %d bytes\n", ilk+1, total_sib_buffer_bytesize[ilk]);

    // WHAT ABOUT THE BIG CHUNKS??? XXX BUGGO FIXME

    // Compute the total size of the pickled datastructure:
    //
    smallchunk_sibs_count = 0;
    total_bytesize   = 0;
    //
    for (int ilk = 0;  ilk < MAX_PLAIN_ILKS;  ilk++) {
	//
	if (total_sib_buffer_bytesize[ilk] > 0) {
	    smallchunk_sibs_count++;
	    total_bytesize += total_sib_buffer_bytesize[ilk];
	}
    }

    total_bytesize
       +=
	sizeof( Heapfile_Header )
        +
	sizeof( Pickle_Header    )
	+
	(smallchunk_sibs_count * sizeof( Sib_Header ));

    // COUNT SPACE FOR BIG CHUNKS

    total_bytesize
       +=
	sizeof(Externs_Header)
        +
	heapfile_cfun_table_bytesize( result->cfun_table );    // Include the space for the external symbols (i.e., runtime C functions referenced within the heapgraph).

    // Allocate the heap bytevector for the pickled
    // datastructure representation and initialize
    // the bytevector-writer.
    //
    pickle
	=
	allocate_heap_ram_for_pickle( task, total_bytesize );
    //
    wr =  WR_OpenMem( PTR_CAST(Unt8*, pickle), total_bytesize );							// WR_OpenMem				def in    src/c/heapcleaner/mem-writer.c

    // Initialize the sib headers:
    //
    sib_header_bytesize =  smallchunk_sibs_count * sizeof(Sib_Header);
    //
    sib_header_buffer        =  (Sib_Header*) MALLOC (sib_header_bytesize);
    //
    p = sib_header_buffer;
    //
    for (int ilk = 0;  ilk < MAX_PLAIN_ILKS;  ilk++) {
        //
	if (total_sib_buffer_bytesize[ ilk ] <= 0) {
	    //
	    sib_headers[ilk] = NULL;
	    //
	} else {
	    //
	    p->age		    	    = 0;
	    p->chunk_ilk	    	    = ilk;
	    //
	    p->info.o.base_address	    = 0;   					// Not used.
	    p->info.o.bytesize	    = total_sib_buffer_bytesize[ ilk ];
	    p->info.o.rounded_bytesize = -1;					// Not used.
	    //
	    p->offset		            = -1;  					// Not used.
	    sib_headers[ ilk ]	            = p;
	    p++;
	}
    }

    // What about big chunks? XXX BUGGO FIXME

    // Write the pickle image header:
    //
    if (heapio__write_image_header (wr, NORMAL_DATASTRUCTURE_PICKLE) == FAILURE) {								// heapio__write_image_header		def in    src/c/heapcleaner/export-heap-stuff.c
	//
	FREE( sib_header_buffer );

	return PICKLER_ERROR;
    }

    // Write the pickle header:
    //	
    {   Pickle_Header	header;

	header.smallchunk_sibs_count     =  smallchunk_sibs_count;
	header.hugechunk_sibs_count      =  0;			// FIX THIS   XXX BUGGO FIXME
	header.hugechunk_ramregion_count =  0;			// FIX THIS   XXX BUGGO FIXME

	if (!IS_EXTERNAL_TAG( root_chunk )) {

	    Sibid sibid =  SIBID_FOR_POINTER( book_to_sibid_global, root_chunk );

	    if (!SIBID_KIND_IS_CODE(sibid)) {

		// This is the normal case  --
		// we're saving a vanilla heap value.

		Punt  addr =  HEAP_POINTER_AS_UNT( root_chunk );

		int age  =  GET_AGE_FROM_SIBID( sibid) - 1;
		int kind =  GET_KIND_FROM_SIBID(sibid) - 1;									// GET_KIND_FROM_SIBID			def in    src/c/h/sibid.h

		addr -= adjust[ age ][ kind ].base;
		addr += adjust[ age ][ kind ].offset;

		header.root_chunk = HIO_TAG_PTR(kind, addr);									// HIO_TAG_PTR				def in    src/c/heapcleaner/runtime-heap-image.h

	    } else {

		//
		Embedded_Chunk_Info*  p
		    =
		    FIND_EMBEDDED_CHUNK( result->embedded_chunk_table, root_chunk );

		if ((p == NULL) || (p->kind == USED_CODE)) {
		    //
		    say_error( "Pickling compiled Mythryl code not implemented\n" );
		    FREE (sib_header_buffer);
		    return PICKLER_ERROR;
		} else {
		    header.root_chunk = p->relocated_address;
		}
	    }

	} else {	// IS_EXTERNAL_TAG( root_chunk )
	    //
	    ASSERT( smallchunk_sibs_count == 0 );

	    header.root_chunk = root_chunk;
	}

	WR_WRITE(wr, &header, sizeof(header));											// WR_WRITE					def in    src/c/heapcleaner/writer.h
	//
	if (WR_ERROR(wr)) {
	    FREE (sib_header_buffer);
	    return PICKLER_ERROR;
	}
    }

    // Record in the pickle the table of heap-referenced
    // runtime C functions.  May also include
    // a handful of assembly fns, exceptions
    // and refcells:
    //
    {   int bytes_written =   heapio__write_cfun_table( wr, result->cfun_table );					// heapio__write_cfun_table			def in    src/c/heapcleaner/export-heap-stuff.c

	if (bytes_written == -1) {
	    FREE( sib_header_buffer );
	    return PICKLER_ERROR;
	}
    }

    // Write the pickle sib headers:
    //
    WR_WRITE (wr, sib_header_buffer, sib_header_bytesize);
    //
    if (WR_ERROR(wr)) {
	FREE (sib_header_buffer);
	return PICKLER_ERROR;
    }

    // Write the pickled datastructure proper:
    //
    for (int ilk = 0;  ilk < MAX_PLAIN_ILKS;  ilk++) {
	//
	if (ilk == STRING_ILK) {

	    // Write into the pickle the required embedded literals:
            //
	    pickler__pickle_embedded_literals( wr );										// pickler__pickle_embedded_literals		def in    src/c/heapcleaner/datastructure-pickler-cleaner.c

	    // Write into the pickle remaining required strings:
            //
	    for (int age = 0;  age < max_age;  age++) {
		//
		Sib* sib = heap->agegroup[ age ]->sib[ ilk ];

		if (sib_is_active(sib)) {											// sib_is_active				def in    src/c/h/heap.h
		    //
		    WR_WRITE(wr, sib->tospace,
			(Punt) sib->next_tospace_word_to_allocate
                       -(Punt) sib->tospace
                    );
		}
	    }

	} else {

	    for (int age = 0;  age < max_age;  age++) {
		//
		Sib* sib = heap->agegroup[ age ]->sib[ ilk ];

		if (sib_is_active( sib )) {
		    //
		    Val*  top =  sib->next_tospace_word_to_allocate;
		    //
		    for (Val*
			p =  sib->tospace;
                        p <  top;
                        p++
		    ){
			Val w =  *p;

			if (IS_POINTER(w)) {
			    //
			    Sibid sibid =  SIBID_FOR_POINTER( book_to_sibid_global, w );

			    if (BOOK_IS_UNMAPPED(sibid)) {
				//
				w =  add_cfun_to_heapfile_cfun_table( result->cfun_table, w);

				ASSERT (w != HEAP_VOID);

			    } else if (SIBID_KIND_IS_CODE(sibid)) {

				Embedded_Chunk_Info*  chunk_info
				    =
				    FIND_EMBEDDED_CHUNK( result->embedded_chunk_table, w );

				if (chunk_info == NULL
				||  chunk_info->kind == USED_CODE
				){
				    die("Pickling of Mythryl compiled code not implemented");
				} else {
				    w = chunk_info->relocated_address;
                                }

			    } else {

			        // Adjust the pointer:
                                //
				int  age  =  GET_AGE_FROM_SIBID( sibid)-1;
				int  kind =  GET_KIND_FROM_SIBID(sibid)-1;

				Punt addr =  HEAP_POINTER_AS_UNT(w);

				addr -=  adjust[ age ][ kind ].base;
				addr +=  adjust[ age ][ kind ].offset;

				w = HIO_TAG_PTR( kind, addr );
			    }
			}								// if (IS_POINTER(w))
			WR_PUT(wr, (Val_Sized_Unt)w);
		    }									// for
		}
	    }
	}
    }

    FREE( sib_header_buffer );

    if (WR_ERROR(wr))	return PICKLER_ERROR;

    SEQHDR_ALLOC (task, pickle, STRING_TAGWORD, pickle, total_bytesize);

    return  pickle;
}											// fun pickle_heap_datastructure


static Val   allocate_heap_ram_for_pickle   (Task*  task,  Punt  bytesize) {
    //       ============================
    //
    // Allocate a bytevector to hold the pickled datastructure.

    Heap* heap   =  task->heap;
    int	  size_in_words =  BYTES_TO_WORDS( bytesize );
    Val	  tagword   =  MAKE_TAGWORD( size_in_words, FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG );

    // We probably should allocate space in the hugechunk region for these chunks.	XXX BUGGO FIXME
    //
    if (bytesize >= heap->agegroup0_buffer_bytesize-(8*ONE_K_BINARY))   die ("Pickling %d bytes not supported -- increase agegroup0 buffer size.", bytesize);	// XXX BUGGO FIXME

    LIB7_AllocWrite( task, 0, tagword );

    return   LIB7_Alloc( task, size_in_words );
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
