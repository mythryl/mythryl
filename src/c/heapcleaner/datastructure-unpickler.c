// datastructure-unpickler.c

#include "../mythryl-config.h"

#include <stdio.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "heap.h"
#include "runtime-heap-image.h"
#include "mythryl-callable-cfun-hashtable.h"
#include "import-heap-stuff.h"


// For background see:  src/A.DATASTRUCTURE-PICKLING.OVERVIEW


static Status   read_image   (Task* task,   Inbuf* bp,   Val* chunk_ref);


Val   unpickle_datastructure   (Task* task,  Unt8* buf,  long len,  Bool* seen_error) {
    //====================== 
    // 
    // Build a heap chunk from a sequence of bytes.
    // The fd is the underlying file descriptor (== -1, if unpickling from a bytevector).
    // buf is any pre-read bytes of data.
    // nbytesP points to the number of bytes in buf.
    //
    // This fn gets exported to the Mythryl level as 'unpickle_datastructure' via
    //
    //        src/c/lib/heap/datastructure-unpickler.c
    // and then
    //        src/lib/std/src/unsafe/unsafe.pkg
    //        

    Inbuf	     inbuf;
    Heapfile_Header  header;
    Val		     chunk;

    inbuf.needs_to_be_byteswapped	= FALSE;
    inbuf.file		= NULL;
    inbuf.base		= buf;
    inbuf.buf		= buf;
    inbuf.nbytes	= len;

    // Read the chunk header:
    //
    if (heapio__read_block( &inbuf, &header, sizeof(header) ) == FALSE) {	// heapio__read_block	def in    src/c/heapcleaner/import-heap-stuff.c
        //
	*seen_error = TRUE;
	return HEAP_VOID;
    }

    if (header.byte_order != ORDER) {
	if (BIGENDIAN_TO_HOST(header.byte_order) != ORDER) {
	    *seen_error = TRUE;
	    return HEAP_VOID;
	}
	header.magic = BIGENDIAN_TO_HOST(header.magic);
	header.kind = BIGENDIAN_TO_HOST(header.kind);
	inbuf.needs_to_be_byteswapped = TRUE;
    }
    if (header.magic != PICKLE_MAGIC) {
	*seen_error = TRUE;
	return HEAP_VOID;
    }

    switch (header.kind) {
        //
    case NORMAL_DATASTRUCTURE_PICKLE:
	if (read_image( task, &inbuf, &chunk ) == FALSE) {			// Defined below
	    *seen_error = TRUE;
	    return HEAP_VOID;
	}
	break;

    case UNBOXED_PICKLE:
	{
	    Pickle_Header	bhdr;

	    if (heapio__read_block( &inbuf, &bhdr, sizeof(bhdr) ) != FALSE) {
		chunk = bhdr.root_chunk;
	    } else {
	        *seen_error = TRUE;
	        return HEAP_VOID;
	    }
	}
	break;

    default:
	*seen_error = TRUE;
	return HEAP_VOID;
    }

    return chunk;
}										// fun unpickle_datastructure


static Status   read_image  (Task* task,  Inbuf* bp,  Val* chunk_ref) {
    //          ==========
    //
    Pickle_Header	pickle_header;
    Val*		externs;

    Sib_Header*	sib_headers[ TOTAL_SIBS ];
    Sib_Header*	sib_headers_buffer;

    int sib_headers_size;

    Agegroup*  age1 =   task->heap->agegroup[ 0 ];

    if (heapio__read_block( bp, &pickle_header, sizeof(pickle_header) ) == FALSE
    ||  pickle_header.smallchunk_sibs_count > MAX_PLAIN_SIBS				// MAX_PLAIN_SIBS		def in    src/c/h/sibid.h
    ||  pickle_header.hugechunk_sibs_count  > MAX_HUGE_SIBS				// MAX_HUGE_SIBS		def in    src/c/h/sibid.h
    ){
	return FALSE;									// XXX BUGGO FIXME we gotta do better than this.
    }

    // Read the externals table:
    //
    externs = heapio__read_externs_table( bp );



    // Read the sib headers:
    //
    sib_headers_size =  (pickle_header.smallchunk_sibs_count + pickle_header.hugechunk_sibs_count)
		        *
                        sizeof( Sib_Header );
    //
    sib_headers_buffer =  (Sib_Header*) MALLOC (sib_headers_size);
    //
    if (heapio__read_block( bp, sib_headers_buffer, sib_headers_size ) == FALSE) {
	//
	FREE( sib_headers_buffer );
	return FALSE;
    }
    //
    for (int ilk = 0;  ilk < TOTAL_SIBS;  ilk++) {
        //
	sib_headers[ ilk ] =  NULL;
    }
    //
    for (int sib = 0;  sib < pickle_header.smallchunk_sibs_count;  sib++) {
        //
	Sib_Header* p =  &sib_headers_buffer[ sib ];
	//
	sib_headers[ p->chunk_ilk ] =  p;
    }



    // DO BIG CHUNK HEADERS TOO

    // Check the heap to see if there is
    // enough free space in agegroup 1:
    //
    {   Punt	agegroup0_buffer_bytesize =   agegroup0_buffer_size_in_bytes( task );
        //
	Bool needs_cleaning  =   FALSE;

	for (int ilk = 0;  ilk < MAX_PLAIN_SIBS;  ilk++) {
	    //
	    Sib* sib = age1->sib[ ilk ];

	    if (sib_headers[ilk] != NULL
		&&
               (!sib_is_active(sib)								// sib_is_active		def in    src/c/h/heap.h
	       || sib_freespace_in_bytes(sib) < sib_headers[ ilk ]->info.o.bytesize		// sib_freespace_in_bytes	def in    src/c/h/heap.h
                                               +
                                               agegroup0_buffer_bytesize
               )
            ){
		needs_cleaning = TRUE;
		sib->requested_extra_free_bytes = sib_headers[ ilk ]->info.o.bytesize;
	    }
	}

	if (needs_cleaning) {
	    //
	    if (bp->nbytes <= 0) {
		//
		call_heapcleaner( task, 1 );							// call_heapcleaner		def in   /src/c/heapcleaner/call-heapcleaner.c

	    } else {
		//
	        // The cleaning may move the buffer, so:
                
		Val buffer =  PTR_CAST( Val,  bp->base );

		{   Roots extra_roots = { &buffer, NULL };
		    //
		    call_heapcleaner_with_extra_roots (task, 1, &extra_roots );
		}

		if (buffer != PTR_CAST( Val,  bp->base )) {
		    //
		    // The buffer moved, so adjust the buffer pointers:

		    Unt8* new_base = PTR_CAST( Unt8*, buffer );

		    bp->buf  = new_base + (bp->buf - bp->base);
		    bp->base = new_base;
		}
            }
	}
    }

    // Read the pickled datastructure elements:
    //
    {   Punt  sib_base[ MAX_PLAIN_SIBS ];
	//
	for (int ilk = 0;  ilk < MAX_PLAIN_SIBS;  ilk++) {
	    //
	    if (sib_headers[ilk] != NULL) {
		//
	        Sib* sib = age1->sib[ ilk ];

	        sib_base[ilk] =  (Punt) sib->tospace.used_end;

	        heapio__read_block( bp, (sib->tospace.used_end), sib_headers[ilk]->info.o.bytesize );

		// debug_say ("[%2d] Read [%#x..%#x)\n", ilk+1, sib->tospace.used_end,
		// (Punt)(sib->tospace.used_end)+sib_headers[ilk]->info.o.bytesize);
	    }
	}

        // Adjust the pointers:
        //
	for (int ilk = 0;  ilk < MAX_PLAIN_SIBS;  ilk++) {
	    //
	    if (sib_headers[ilk] != NULL) {
		//
		Sib* sib = age1->sib[ ilk ];
		//
		if (ilk == NONPTR_DATA_SIB) {
		    //
		    sib->tospace.used_end = (Val*) ((Punt)(sib->tospace.used_end)
			      + sib_headers[ilk]->info.o.bytesize);
		} else {

		    Val* p = sib->tospace.used_end;

		    Val* stop = (Val*) ((Punt)p + sib_headers[ilk]->info.o.bytesize);

		    while (p < stop) {
			//
		        Val w = *p;

		        if (! IS_TAGGED_INT(w)) {
			    //
			    if (IS_EXTERNAL_TAG(w)) {
				//
			        w = externs[ EXTERNAL_ID( w ) ];
				//
			    } else if (! IS_TAGWORD( w )) {
				//
				// debug_say ("adjust (@%#x) %#x --> ", p, w);

			        w = PTR_CAST( Val,  sib_base[ HIO_GET_ID(w) ] + HIO_GET_OFFSET(w) );

				// debug_say ("%#x\n", w);
			    }
		            *p = w;
		        }
		        p++;
		    }
		    sib->tospace.used_end	=
		    sib->tospace.swept_end	= stop;
	        }
	    }
	}

	// Adjust the root-chunk pointer:
	//
	if (IS_EXTERNAL_TAG(pickle_header.root_chunk)) {
	    //	
	    *chunk_ref = externs[ EXTERNAL_ID( pickle_header.root_chunk ) ];

	} else {

	    *chunk_ref
		=
		PTR_CAST(  Val,

		    sib_base[ HIO_GET_ID( pickle_header.root_chunk ) ]
		    +
                    HIO_GET_OFFSET(       pickle_header.root_chunk )
		);
	}

	// debug_say( "root = %#x, adjusted = %#x\n", pickle_header.root_chunk, *chunk_ref );
    }

    FREE( sib_headers_buffer );
    FREE( externs );

    return TRUE;
}							 // fun read_image


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

