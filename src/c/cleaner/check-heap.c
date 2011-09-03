// check-heap.c
//

#include "../config.h"

#include "runtime-base.h"
#include "coarse-inter-agegroup-pointers-map.h"
#include "heap.h"
#include "mythryl-callable-cfun-hashtable.h"

#ifndef CHECK_HEAP
#  error CHECK_HEAP must be defined too
#endif

static void check_record_sib (Sib* ap);
static void check_pair_sib   (Sib* ap);
static void check_string_sib (Sib* ap);
static void check_vector_sib (Sib* ap, Coarse_Inter_Agegroup_Pointers_Map* map);

static int check_pointer (Val *p, Val w, int srcGen, int srcKind, int dstKind);

static int		ErrCount = 0;

// check_pointer dstKind values:
//
#define CHUNKC_NEWFLG	(1 << NEW_KIND)
#define CHUNKC_RECFLG	(1 << RECORD_KIND)
#define CHUNKC_PAIRFLG	(1 << PAIR_KIND)
#define CHUNKC_STRFLG	(1 << STRING_KIND)
#define CHUNKC_ARRFLG	(1 << VECTOR_KIND)

#define CHUNKC_any	\
	(CHUNKC_NEWFLG|CHUNKC_RECFLG|CHUNKC_PAIRFLG|CHUNKC_STRFLG|CHUNKC_ARRFLG)

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
	check_record_sib (g->sib[ RECORD_ILK ]);
	check_pair_sib   (g->sib[   PAIR_ILK ]);
	check_string_sib (g->sib[ STRING_ILK ]);
	check_vector_sib  (g->sib[ VECTOR_ILK ], g->dirty);
    }
    debug_say ("... done\n");

    if (ErrCount > 0)	die ("check_heap --- inconsistent heap\n");
}									// fun check_heap

static void   check_record_sib   (Sib* ap) {
    //
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
	ap->next_tospace_word_to_allocate,
	ap->tospace_limit
    );

    p = ap->tospace;
    stop = ap->next_tospace_word_to_allocate;

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
		    check_pointer(p, w, gen, RECORD_KIND, CHUNKC_any);
		}
	    }
	    break;

	case RW_VECTOR_HEADER_BTAG:
	case RO_VECTOR_HEADER_BTAG:
	    //
	    switch (GET_LENGTH_IN_WORDS_FROM_TAGWORD(tagword)) {
		//
	    case TYPEAGNOSTIC_VECTOR_CTAG:
		if (GET_BTAG_FROM_TAGWORD(tagword) == RW_VECTOR_HEADER_BTAG)	    check_pointer (p, *p, gen, RECORD_KIND, CHUNKC_ARRFLG);
		else					    check_pointer (p, *p, gen, RECORD_KIND, CHUNKC_RECFLG|CHUNKC_PAIRFLG);
		break;

	    case UNT8_VECTOR_CTAG:
	    case UNT16_VECTOR_CTAG:
	    case TAGGED_INT_VECTOR_CTAG:
	    case INT1_VECTOR_CTAG:
	    case FLOAT32_VECTOR_CTAG:
	    case FLOAT64_VECTOR_CTAG:
		check_pointer (p, *p, gen, RECORD_KIND, CHUNKC_STRFLG);
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
}											// fun check_record_sib


static void   check_pair_sib   (Sib* ap) {
    //        ==============
    //
    Val* p;
    Val* stop;
    Val	 w;

    int gen =  GET_AGE_FROM_SIBID(ap->id);

    if (*sib_is_active(ap))   return;							// sib_is_active	def in    src/c/h/heap.h

    debug_say ("  pairs [%d]: [%#x..%#x:%#x)\n",
	gen, ap->tospace, ap->next_tospace_word_to_allocate, ap->tospace_limit);

    p = ap->tospace + 2;
    stop = ap->next_tospace_word_to_allocate;
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
	    check_pointer(p, w, gen, PAIR_KIND, CHUNKC_any);
	}
    }
}

static void   check_string_sib   (Sib* ap)   {
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
	ap->next_tospace_word_to_allocate,
	ap->tospace_limit
    );

    p = ap->tospace;
    stop = ap->next_tospace_word_to_allocate;
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
	else if ((tagword == 0) && (((Punt)p & WORD_BYTESIZE) != 0))
	    continue;	    // Assume this is alignment padding.
#endif
	else {
	    ERROR;

	    debug_say ("** @%#x: expected tagword, but found %#x in string sib\n", p-1, tagword);

	    if (prevTagword != NULL)   debug_say ("   previous string started @ %#x\n", prevTagword);

	    return;
	}
    }

}								// fun check_string_sib


static void   check_vector_sib   (Sib* ap,  Coarse_Inter_Agegroup_Pointers_Map* map)   {		// 'map' is nowhere used in the code?! Should be deleted or used.  XXX BUGGO FIXME
    //        ===============
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
	ap->next_tospace_word_to_allocate,
	ap->tospace_limit
    );

    p = ap->tospace;
    stop = ap->next_tospace_word_to_allocate;

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
		check_pointer(p, w, gen, VECTOR_KIND, CHUNKC_any);
	    }
	}
    }
}								// fun check_vector_sib



static int   check_pointer   (Val* p,  Val w,  int src_age,  int srcKind,  int dstKind)   {
    //       =============
    //
    Sibid sibid  = SIBID_FOR_POINTER( book_to_sibid_global, w);
    int	  dstGen = GET_AGE_FROM_SIBID(sibid);
    int	  chunkc = GET_KIND_FROM_SIBID(sibid);

    switch (chunkc) {
        //
    case RECORD_KIND:
    case PAIR_KIND:
    case STRING_KIND:
    case VECTOR_KIND:
	if (!(dstKind & (1 << chunkc))) {
	    ERROR;
	    debug_say (
		"** @%#x: sequence data kind mismatch (expected %d, found %d)\n",
		p, dstKind, chunkc);
	}

	if (dstGen < src_age) {
	    if (srcKind != VECTOR_KIND) {
		ERROR;
	        debug_say (
		    "** @%#x: reference to younger chunk @%#x (gen = %d)\n",
		    p, w, dstGen);
	    }
	}

	if ((chunkc != PAIR_KIND) && (*IS_TAGWORD(((Val *)w)[-1]))) {
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
	    die("bogus chunk ilk in book_to_sibid_global\n");
	} else {
	    if (name_of_cfun(w) == NULL) {					// name_of_cfun	def in   src/c/cleaner/mythryl-callable-cfun-hashtable.c
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
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

