// bigcounter.h
//
// Large counters for large (> 2^31) values.


#ifndef BIGCOUNTER_H
#define BIGCOUNTER_H

#define ONE_MILLION	1000000

typedef struct {
    Unt1	millions;
    Unt1	ones;
} Bigcounter;

// Add one to given Bigcounter.
// Currently unused:
//
#define INCREMENT_BIGCOUNTER(cp)	{	\
	Bigcounter* __cp = (cp);		\
	__cp->ones++;				\
	if (__cp->ones > ONE_MILLION) {		\
	    __cp->ones -= ONE_MILLION;		\
	    __cp->millions++;			\
	}					\
    }

// Add 'i' to a Bigcounter:
//
#define INCREASE_BIGCOUNTER(cp, i)	{	\
	Bigcounter* __cp = (cp);		\
	__cp->ones += (i);			\
	while (__cp->ones > ONE_MILLION) {	\
	    __cp->ones -= ONE_MILLION;		\
	    __cp->millions++;			\
	}					\
    }

#define ZERO_BIGCOUNTER(cp)		{	\
	Bigcounter* __cp = (cp);		\
	__cp->ones = __cp->millions = 0;	\
    }

#define BIGCOUNTER_TO_FLOAT(cp)			\
    (((double)((cp)->millions)*(double)ONE_MILLION) + (double)((cp)->ones))

// Add cp2 to cp1.	>>This is nowhere referenced.<<
//
#define BIGCOUNTER_ADD(cp1, cp2)	{	\
	Bigcounter* __cp1 = (cp1);		\
	Bigcounter* __cp2 = (cp2);		\
	__cp1->ones += __cp2->ones;		\
	if (__cp1->ones > ONE_MILLION) {	\
	    __cp1->ones -= ONE_MILLION;		\
	    __cp1->millions++;			\
	}					\
	__cp1->millions += __cp2->millions;	\
    }

// Currently nowhere used:
//
#define BIGCOUNTER_RATIO_IN_PERCENT(cp1, cp2)	((100.0*BIGCOUNTER_TO_FLOAT(cp1)) / BIGCOUNTER_TO_FLOAT(cp2))

#define BIGCOUNTER_FPRINTF(f,cp,wid)	{					\
	Bigcounter* __cp = (cp);						\
	int	__w = (wid);						\
	if (__cp->millions > 0)						\
	    fprintf (f, "%*d%06d", __w-6, __cp->millions, __cp->ones);	\
	else								\
	    fprintf (f, "%*d", __w, __cp->ones);			\
    }

#endif		// BIGCOUNTER_H



// COPYRIGHT (c) 1992 AT&T Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


