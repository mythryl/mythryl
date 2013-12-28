// copy-loop.h
//
// A dirty, but quick, copy loop for the heapcleaner.
// This is used in:
//
//     src/c/heapcleaner/datastructure-pickler-cleaner.c
//     src/c/heapcleaner/heapclean-n-agegroups.c
//     src/c/heapcleaner/heapclean-agegroup0.c


#ifndef COPY_LOOP_H
#define COPY_LOOP_H

#define COPYLOOP(SRC,DST,LEN)	{				\
	Vunt	*__src = (Vunt *)(SRC);				\
	Vunt	*__dst = (Vunt *)(DST);				\
	int	__len = (LEN);					\
	int	__m;						\
	switch (__len & 0x3) {					\
	  case 3: *__dst++ = *__src++;				\
	  case 2: *__dst++ = *__src++;				\
	  case 1: *__dst++ = *__src++;				\
	  case 0: break;					\
	}							\
	__m = __len >> 2;					\
	while (--__m >= 0) {					\
	    __dst[0] = __src[0];				\
	    __dst[1] = __src[1];				\
	    __dst[2] = __src[2];				\
	    __dst[3] = __src[3];				\
	    __dst += 4;						\
	    __src += 4;						\
	}							\
    }



#endif // COPY_LOOP_H



// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.


