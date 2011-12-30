// tuple-ops.c
//
// Some (type unsafe) operations on records.


/*
###       "Old C programmers never die,
###        they just get cast into void."
###                    -- Anonymous
*/

#include "../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "sibid.h"
#include "heapcleaner.h"


static int   GetLen   (Val r)   {
    //
    // Check that we really have a record chunk, and return its length.

    Val	d;
    int t;

    if (! IS_POINTER(r))	return -1;

    switch (GET_KIND_FROM_SIBID( SIBID_FOR_POINTER( book_to_sibid__global, r) ) ) {
	//
    case NEW_KIND:
	d = CHUNK_TAGWORD(r);
	t = GET_BTAG_FROM_TAGWORD(d);
	//
	if (t == PAIRS_AND_RECORDS_BTAG)   return GET_LENGTH_IN_WORDS_FROM_TAGWORD( d );
	else	 			   return -1;

    case PAIR_KIND:
	return 2;

    case RECORD_KIND:
	d = CHUNK_TAGWORD(r);
	t = GET_BTAG_FROM_TAGWORD(d);
	if (t == PAIRS_AND_RECORDS_BTAG)  return GET_LENGTH_IN_WORDS_FROM_TAGWORD(d);
	else				  return -1;

    default:
	return -1;
    }
}



Val   concatenate_two_tuples   (Task* task,  Val r1,  Val r2)   {
    //======================
    //
    // Concatenate two records.
    // Returns Void if either argument is not
    // a record of length at least one.
    //
    // This function is invoked (only) from    src/c/lib/heap/concatenate-two-tuples.c

									    ENTER_MYTHRYL_CALLABLE_C_FN("concatenate_two_tuples");

    int		l1 = GetLen(r1);
    int		l2 = GetLen(r2);

    if ((l1 > 0)
    &&  (l2 > 0)
    ){

	int		n = l1+l2;
	int		i;
	int		j;
	Val*	p;

	LIB7_AllocWrite (task, 0, MAKE_TAGWORD(n, PAIRS_AND_RECORDS_BTAG));

	j = 1;

	for (i = 0, p = PTR_CAST(Val*, r1);  i < l1;  i++, j++) {
	    //
	    LIB7_AllocWrite (task, j, p[i]);
	}

	for (i = 0, p = PTR_CAST(Val*, r2);  i < l2;  i++, j++) {
	    //
	    LIB7_AllocWrite (task, j, p[i]);
	}

	return LIB7_Alloc(task, n);

    } else {

	return HEAP_VOID;
    }
}



// COPYRIGHT (c) 1998 Bell Labs, Lucent Technologies.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


