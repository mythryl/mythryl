// get-chunk-age.c

#include "../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "heap.h"
#include "heapcleaner.h"

int   get_chunk_age   (Val chunk) {
    //============= 
    // 
    // Get the agegroup of a chunk.
    // Return -1 for external/unboxed chunks.
    //
    // We are called (only) from
    //     src/c/heapcleaner/datastructure-pickler.c	

    if (! IS_POINTER( chunk )) {
	return -1;
    } else {
        //
	Sibid aid =  SIBID_FOR_POINTER( book_to_sibid__global, chunk );
        //
	if (SIBID_KIND_IS_CODE( aid )) {
	    //	
	    Hugechunk_Region*	   region;
	    Hugechunk* dp;

	    int  i;
	    for (i = GET_BOOK_CONTAINING_POINTEE(chunk);  !SIBID_ID_IS_BIGCHUNK_RECORD(aid);  aid = book_to_sibid__global[--i]) {
		continue;
	    }

	    region = (Hugechunk_Region *)ADDRESS_OF_BOOK(i);
	    dp = get_hugechunk_holding_pointee(region, chunk);

	    return dp->age;

	} else if (aid == AGEGROUP0_SIBID) {	    return  0;
	} else if (BOOK_IS_UNMAPPED(aid)) {	    return -1;
	} else {	 		    return  GET_AGE_FROM_SIBID( aid );
	}
    }

}



// COPYRIGHT (c) 1993 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

