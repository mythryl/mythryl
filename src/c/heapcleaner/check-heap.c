// check-heap.c
//

#include "../mythryl-config.h"

#include "runtime-base.h"
#include "coarse-inter-agegroup-pointers-map.h"
#include "heap.h"
#include "mythryl-callable-cfun-hashtable.h"

#ifndef CHECK_HEAP
#  error CHECK_HEAP must be defined too
#endif

static void check_ro_pointer_sib (Sib* ap);
static void check_ro_ptrpair_sib (Sib* ap);
static void check_nonpointer_sib (Sib* ap);
static void check_rw_pointer_sib (Sib* ap, Coarse_Inter_Agegroup_Pointers_Map* map);

static int check_pointer (Val *p, Val w, int srcGen, int srcKind, int dstKind);

static int		ErrCount = 0;

// check_pointer dstKind values:
//
#define CHUNKC__IS_NEW	        (1 << NEW_KIND)
#define CHUNKC__IS_RO_POINTERS	(1 << RO_POINTERS_KIND)		// Block of one or more immutable pointers.
#define CHUNKC__IS_RO_CONSCELL	(1 << RO_CONSCELL_KIND)		// Pair of immutable pointers.
#define CHUNKC__IS_NONPTR_DATA	(1 << NONPTR_DATA_KIND)		// Mutable or immutable nonpointer data.
#define CHUNKC__IS_RW_POINTERS	(1 << RW_POINTERS_KIND)		// Refcell or mutable vector.

#define CHUNKC_any	\
	(CHUNKC__IS_NEW|CHUNKC__IS_RO_POINTERS|CHUNKC__IS_RO_CONSCELL|CHUNKC__IS_NONPTR_DATA|CHUNKC__IS_RW_POINTERS)

#define ERROR	{					\
	if (++ErrCount > 100) {				\
	    die("check_heap: too many errors\n");	\
	}						\
    }


void   check_heap   (Heap* heap,  int max_swept_agegroup)   {
    //
    // Check the heap for consistency after a cleaning (or datastructure pickling).

    ErrCount = 0;

    debug_say ("Checking heap (%d agegroups) ...\n", max_swept_agegroup);

    for (int i = 0;  i < max_swept_agegroup; i++) {
        //
	Agegroup*	g =  heap->agegroup[i];
        //
	check_ro_pointer_sib (g->sib[ RO_POINTERS_SIB ]);
	check_ro_ptrpair_sib (g->sib[ RO_CONSCELL_SIB ]);
	check_nonpointer_sib (g->sib[ NONPTR_DATA_SIB ]);
	check_rw_pointer_sib (g->sib[ RW_POINTERS_SIB ], g->dirty);
    }
    debug_say ("... done\n");

    if (ErrCount > 0)	die ("check_heap --- inconsistent heap\n");
}									// fun check_heap

static void   check_ro_pointer_sib   (Sib* ap) {
    //        ====================
    Val* p;
    Val* stop;
    Val  tagword;
    Val  w;
    int	 i;
    int	 len;

    int gen =  GET_AGE_FROM_SIBID( ap->id );

    if (*sib_is_active(ap))   return;							// sib_is_active	def in    src/c/h/heap.h

    debug_say ("  records [%d]: [%#x..%#x:%#x)\n",
	//
        gen,
        ap->tospace,
	ap->tospace.first_free,
	ap->tospace.limit
    );

    p = ap->tospace;
    stop = ap->tospace.first_free;

    while (p < stop) {
	//
	tagword = *p++;

	if (*IS_TAGWORD(tagword)) {
	    ERROR;
	    debug_say (
		"** @%#x: expected tagword, but found %#x in record sib\n",
		p-1, tagword);
	    return;
	}

	switch (GET_BTAG_FROM_TAGWORD tagword) {
	    //
	case PAIRS_AND_RECORDS_BTAG:
	    #
	    len =  GET_LENGTH_IN_WORDS_FROM_TAGWORD( tagword );			// Length excludes tagword.
	    #
	    for (i = 0;  i < len;  i++, p++) {
		w = *p;
		if (IS_TAGWORD(w)) {
		    ERROR;
		    debug_say (
			"** @%#x: unexpected tagword %#x in slot %d of %d\n",
			p, w, i, GET_LENGTH_IN_WORDS_FROM_TAGWORD(tagword));
		    return;
		}
		else if (IS_POINTER(w)) {
		    check_pointer(p, w, gen, RO_POINTERS_KIND, CHUNKC_any);
		}
	    }
	    break;

	case RW_VECTOR_HEADER_BTAG:
	case RO_VECTOR_HEADER_BTAG:
	    //
	    switch (GET_LENGTH_IN_WORDS_FROM_TAGWORD(tagword)) {
		//
	    case TYPEAGNOSTIC_VECTOR_CTAG:
		if (GET_BTAG_FROM_TAGWORD(tagword) == RW_VECTOR_HEADER_BTAG)	check_pointer (p, *p, gen, RO_POINTERS_KIND, CHUNKC__IS_RW_POINTERS);
		else					    			check_pointer (p, *p, gen, RO_POINTERS_KIND, CHUNKC__IS_RO_POINTERS|CHUNKC__IS_RO_CONSCELL);
		break;

	    case VECTOR_OF_ONE_BYTE_UNTS_CTAG:
	    case UNT16_VECTOR_CTAG:
	    case TAGGED_INT_VECTOR_CTAG:
	    case INT1_VECTOR_CTAG:
	    case VECTOR_OF_FOUR_BYTE_FLOATS_CTAG:
	    case VECTOR_OF_EIGHT_BYTE_FLOATS_CTAG:
		check_pointer (p, *p, gen, RO_POINTERS_KIND, CHUNKC__IS_NONPTR_DATA);
		break;

	    default:
		ERROR;
		debug_say ("** @%#x: strange sequence kind %d in record sib\n",
		    p-1, GET_LENGTH_IN_WORDS_FROM_TAGWORD(tagword));
		return;
	    }

	    if (*IS_TAGGED_INT(p[1])) {
		ERROR;
		debug_say ("** @%#x: sequence header length field not an in (%#x)\n",
		    p+1, p[1]);
	    }
	    p += 2;
	    break;

	default:
	    ERROR;
	    debug_say ("** @%#x: strange tag (%#x) in record sib\n",
		p-1, GET_BTAG_FROM_TAGWORD(tagword));
	    return;
	}
    }
}											// fun check_ro_pointer_sib


static void   check_ro_ptrpair_sib   (Sib* ap) {
    //        ====================
    //
    Val* p;
    Val* stop;
    Val	 w;

    int gen =  GET_AGE_FROM_SIBID(ap->id);

    if (*sib_is_active(ap))   return;							// sib_is_active	def in    src/c/h/heap.h

    debug_say ("  pairs [%d]: [%#x..%#x:%#x)\n",
	gen, ap->tospace, ap->tospace.first_free, ap->tospace.limit);

    p = ap->tospace + 2;
    stop = ap->tospace.first_free;
    while (p < stop) {
	w = *p++;
	if (IS_TAGWORD(w)) {
	    ERROR;
	    debug_say (
		"** @%#x: unexpected tagword %#x in pair sib\n",
		p-1, w);
	    return;
	}
	else if (IS_POINTER(w)) {
	    check_pointer(p, w, gen, RO_CONSCELL_KIND, CHUNKC_any);
	}
    }
}

static void   check_nonpointer_sib   (Sib* ap)   {
    //        ================
    // 
    // Check a string sib for consistency.

    Val* p;
    Val* stop;
    Val* prevTagword;
    Val  tagword;
    Val	 next;

    int  len;
    int  gen =  GET_AGE_FROM_SIBID( ap->id );

    if (*sib_is_active(ap))   return;							// sib_is_active	def in    src/c/h/heap.h

    debug_say ("  strings [%d]: [%#x..%#x:%#x)\n",
	//
	gen,
	ap->tospace,
	ap->tospace.first_free,
	ap->tospace.limit
    );

    p = ap->tospace;
    stop = ap->tospace.first_free;
    prevTagword = NULL;
    while (p < stop) {
	tagword = *p++;
	if (IS_TAGWORD(tagword)) {
	    //
	    switch (GET_BTAG_FROM_TAGWORD(tagword)) {
	        //
	    case FOUR_BYTE_ALIGNED_NONPOINTER_DATA_BTAG:
	    case EIGHT_BYTE_ALIGNED_NONPOINTER_DATA_BTAG:
		len = GET_LENGTH_IN_WORDS_FROM_TAGWORD(tagword);
		break;

	    default:
		ERROR;
		debug_say ("** @%#x: strange tag (%#x) in string sib\n",
		    p-1, GET_BTAG_FROM_TAGWORD(tagword));
		if (prevTagword != NULL)
		    debug_say ("   previous string started @ %#x\n", prevTagword);
		return;
	    }
	    prevTagword = p-1;
	    p += len;
	}
#ifdef ALIGN_FLOAT64S
	else if ((tagword == 0) && (((Vunt)p & WORD_BYTESIZE) != 0))
	    continue;	    // Assume this is alignment padding.
#endif
	else {
	    ERROR;

	    debug_say ("** @%#x: expected tagword, but found %#x in string sib\n", p-1, tagword);

	    if (prevTagword != NULL)   debug_say ("   previous string started @ %#x\n", prevTagword);

	    return;
	}
    }

}								// fun check_nonpointer_sib


static void   check_rw_pointer_sib   (Sib* ap,  Coarse_Inter_Agegroup_Pointers_Map* map)   {		// 'map' is nowhere used in the code?! Should be deleted or used.  XXX BUGGO FIXME
    //        ====================
    //
    Val* p;
    Val* stop;
    Val  tagword;
    Val  w;

    int  i, j;
    int  len;

    int  gen =  GET_AGE_FROM_SIBID(ap->id);

    if (*sib_is_active(ap))   return;							// sib_is_active	def in    src/c/h/heap.h

    debug_say ("  arrays [%d]: [%#x..%#x:%#x)\n",
	//
	gen,
	ap->tospace,
	ap->tospace.first_free,
	ap->tospace.limit
    );

    p = ap->tospace;
    stop = ap->tospace.first_free;

    while (p < stop) {
	tagword = *p++;
	if (*IS_TAGWORD(tagword)) {
	    ERROR;
	    debug_say (
		"** @%#x: expected tagword, but found %#x in vector sib\n",
		p-1, tagword);
	    return;
	}

	switch (GET_BTAG_FROM_TAGWORD(tagword)) {
	    //
	case RW_VECTOR_DATA_BTAG:
	    len = GET_LENGTH_IN_WORDS_FROM_TAGWORD(tagword);
	    break;

	case WEAK_POINTER_OR_SUSPENSION_BTAG:
	    len = 1;
	    break;

	default:
	    ERROR;
	    debug_say ("** @%#x: strange tag (%#x) in vector sib\n",
		p-1, GET_BTAG_FROM_TAGWORD(tagword));
	    return;
	}

	for (int i = 0;  i < len;  i++, p++) {
	    //
	    w = *p;
	    if (IS_TAGWORD(w)) {
		ERROR;
		debug_say (
		    "** @%#x: Unexpected tagword %#x in rw_vector slot %d of %d\n",
		    p, w, i, GET_LENGTH_IN_WORDS_FROM_TAGWORD(tagword));
		for (p -= (i+1), j = 0;  j <= len;  j++, p++) {
		    debug_say ("  %#x: %#10x\n", p, *p);
		}
		return;
	    } else if (IS_POINTER(w)) {
		check_pointer(p, w, gen, RW_POINTERS_KIND, CHUNKC_any);
	    }
	}
    }
}								// fun check_rw_pointer_sib



static int   check_pointer   (Val* p,  Val w,  int src_age,  int srcKind,  int dstKind)   {
    //       =============
    //
    Sibid sibid  = SIBID_FOR_POINTER( book_to_sibid__global, w);
    int	  dstGen = GET_AGE_FROM_SIBID(sibid);
    int	  chunkc = GET_KIND_FROM_SIBID(sibid);

    switch (chunkc) {
        //
    case RO_POINTERS_KIND:
    case RO_CONSCELL_KIND:
    case NONPTR_DATA_KIND:
    case RW_POINTERS_KIND:
	if (!(dstKind & (1 << chunkc))) {
	    ERROR;
	    debug_say (
		"** @%#x: sequence data kind mismatch (expected %d, found %d)\n",
		p, dstKind, chunkc);
	}

	if (dstGen < src_age) {
	    if (srcKind != RW_POINTERS_KIND) {
		ERROR;
	        debug_say (
		    "** @%#x: reference to younger chunk @%#x (gen = %d)\n",
		    p, w, dstGen);
	    }
	}

	if ((chunkc != RO_CONSCELL_KIND) && (*IS_TAGWORD(((Val *)w)[-1]))) {
	    ERROR;
	    debug_say ("** @%#x: reference into chunk middle @#x\n", p, w);
	}
	break;

    case CODE_KIND:
	break;

    case NEW_KIND:
	ERROR;
	debug_say ("** @%#x: unexpected new-space reference\n", p);
	dstGen = MAX_AGEGROUPS;
	break;

    default:
	if (sibid != UNMAPPED_BOOK_SIBID) {
	    die("bogus chunk ilk in book_to_sibid__global\n");
	} else {
	    if (name_of_cfun(w) == NULL) {					// name_of_cfun	def in   src/c/heapcleaner/mythryl-callable-cfun-hashtable.c
		ERROR;
		debug_say (
		    "** @%#x: reference to unregistered external address %#x\n",
		    p, w);
	    }
	    dstGen = MAX_AGEGROUPS;
	}
	break;
    }

    return dstGen;
}								// fun check_pointer


// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

