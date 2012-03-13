// coarse-inter-agegroup-pointers-map.h
//
// For nomenclature, motivation and overview see:
//
//     src/A.HEAP.OVERVIEW
//


#ifndef INTER_AGEGROUP_POINTERS_MAP_H
#define INTER_AGEGROUP_POINTERS_MAP_H


/////////////////////////////////////////
// Coarse_Inter_Agegroup_Pointers_Map
// ======================================
// 
typedef struct {
    //
    Val*	   base_address;			// The base address of the ram-region covered by the cardmap.
    Vunt  card_count;				// The number of cards covered by the cardmap.
    int	           map_bytesize;			// The number of bytes allocated for this cardmap.
    Unt8	   min_age[ WORD_BYTESIZE ];	// The card map proper -- one min-age value per card.
    //
} Coarse_Inter_Agegroup_Pointers_Map;

#define CLEAN_CARD	0xff				// Any value greater than any real agegroup number.

// We use 256-byte cards:
//
#define LOG2_CARD_BYTESIZE	    8
#define      CARD_BYTESIZE	    (1<<LOG2_CARD_BYTESIZE)
#define      CARD_SIZE_IN_WORDS	    (CARD_BYTESIZE / WORD_BYTESIZE)
//
#define CARDMAP_BYTESIZE(n)   (sizeof(Coarse_Inter_Agegroup_Pointers_Map) + ((((n)+(WORD_BYTESIZE-1)) >> LOG2_BYTES_PER_WORD)-1)*WORD_BYTESIZE)

// Given a ram region divided into cards,
// and a pointer into that region, which
// card is the address in?  This is just:
//
//     (pointer - region_base_address) / cardsize
//
#define POINTER_TO_CARD_INDEX(cardmap, addr)						\
    (((Punt)(addr) - (Punt)(cardmap->base_address)) >> LOG2_CARD_BYTESIZE)


// Given a pointer into the ram-region
// covered by a cardmap, return the min-age
// value of the corresponding card.
//
// This is used exactly once, in
//     src/c/heapcleaner/heapclean-n-agegroups.c 
//
#define CARDMAP_MIN_AGE_VALUE_FOR_POINTER( cardmap, pointer )				\
	    /**/									\
	    ((cardmap)->min_age[ POINTER_TO_CARD_INDEX( cardmap, pointer ) ])

// Set min-age for card containing 'pointer'
// to minimum of 'age' and its previous value:
//
#define MAYBE_UPDATE_CARD_MIN_AGE_PER_POINTER( cardmap, pointer, age )	{		\
	    /**/									\
	    Coarse_Inter_Agegroup_Pointers_Map* __cardmap = (cardmap);		\
	    int	    __i = POINTER_TO_CARD_INDEX(__cardmap, (pointer));			\
	    int	    __g = (age);							\
	    if (__g < __cardmap->min_age[__i])						\
		__cardmap->min_age[__i] = __g;						\
}

// Test a card to see if it is clean:
//
#define CARD_IS_DIRTY( cardmap, index, max_age )					\
            /**/									\
	    ((cardmap)->min_age[ index ] <= (max_age))


#ifdef COUNT_COARSE_INTER_AGEGROUP_POINTERS_MAP_CARDS					// "COUNT_COARSE_INTER_AGEGROUP_POINTERS_MAP_CARDS" switches code on/off in    src/c/heapcleaner/heapclean-n-agegroups.c
    #define COUNT_DIRTY(index_var)	\
	if(__cardmap->min_age[ index_var ] != CLEAN_CARD) cardCnt2[i]++
#else
    #define COUNT_DIRTY(index_var)	/* null */
#endif


// Find all "cards" (1KB blocks) in a sib buffer which
// contain pointers into agegroups no older than max_age.
// We call these "dirty" cards.
// 
// The argument index_var should be an integer variable;
// it is used to pass the index of dirty cards to cmd.
//
// This gets called in two files:
//
//     src/c/heapcleaner/heapclean-n-agegroups.c
//     src/c/heapcleaner/datastructure-pickler-cleaner.c
//
#define FOR_ALL_DIRTY_CARDS_IN_CARDMAP( cardmap, max_age, index_var, cmd )	{	\
    /**/										\
    Coarse_Inter_Agegroup_Pointers_Map* __cardmap = (cardmap);				\
    /**/										\
    int	    __card_count = __cardmap->card_count;					\
    int	    __max_age = (max_age);							\
    /**/										\
    for (index_var = 0;  index_var < __card_count;  index_var++) {			\
        /**/										\
	COUNT_DIRTY( index_var );							\
        /**/										\
	if (CARD_IS_DIRTY(__cardmap, index_var, __max_age)) {				\
	    cmd										\
	}										\
    }											\
}


#endif // INTER_AGEGROUP_POINTERS_MAP_H


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


