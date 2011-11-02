// runtime-configuration.h
//
// Various limits and default settings for the Lib7 run-time system.


#ifndef RUNTIME_CONFIGURATION_H
#define RUNTIME_CONFIGURATION_H

#include "../mythryl-config.h"
#include "runtime-base.h"

#define DEFAULT_IMAGE				NULL						// NULL means: Try to find in-core image using dlopen/dlsym.

#define MAX_LENGTH_FOR_A_BOOTFILE_PATHNAME	 512
#define MAX_NUMBER_OF_BOOT_FILES		1024


#define DEFAULT_ACTIVE_AGEGROUPS	5

#define MAX_ACTIVE_AGEGROUPS	14								// Should agree with MAX_AGEGROUPS in  sibid.h.

#define DEFAULT_OLDEST_AGEGROUP_KEEPING_IDLE_FROMSPACE_BUFFERS	2				// Keep idle fromspace regions for ages 1 & 2

#define DEFAULT_AGEGROUP0_BUFFER_BYTESIZE	(256 * ONE_K_BINARY)				// Size-in-bytes for the per-core (well, per-pthread)
												// generation-zero heap buffer.  The 256KB value is
												// ancient (1992?), but at the moment Intel level-two
												// cache sizes seem to range from 256KB to 1024MB, so
												// 256KB still seems a good choice.
												//                         -- 2011-11-01 CrT

#define DEFAULT_RATIO1	20									// agegroup-one sib buffers are small.
#define DEFAULT_RATIO2	10
#define DEFAULT_RATIO	5
#define MAX_SZ1(NSZ)	(6*(NSZ))


#define CODECHUNK_ALLOCATION_AGEGROUP	2							// Agegroup in which to allocate code chunks.

#define MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS	512
    //
    // Size-in-words of a "small chunk."
    // The C allocation routines allocate
    // small chunks in agegroup0,
    // while large chunks are allocated in
    // agegroup 1.

#define MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER	  ((1024 + 128) * WORD_BYTESIZE)		// Referenced only in   src/c/h/heap.h
    //
    // Size-in-bytes of the allocation buffer.
    // If A is the value of the limit pointer,
    // then A[ ALLOCATION_BUFFER_SIZE_IN_WORDS-1 ]
    // is the address of the next store-vector location.


#define  MAX_C_HEAPCLEANER_ROOTS	16								// Maximum number of global C variables that can be heapcleaner ("garbage collector") roots.

#define  MAX_EXTRA_HEAPCLEANER_ROOTS	16								// Maximum number of additional roots that can be passed to heapcleaner.

// Number of potential cleaner roots.
// This includes space for C global roots,
// Mythryl roots and the terminating null pointer.
//
#ifdef N_PSEUDO_REGS
    #define N_PSEUDO_ROOTS	N_PSEUDO_REGS
#else
    #define N_PSEUDO_ROOTS	0
#endif

#if NEED_PTHREAD_SUPPORT
    // 
    // We must assume that all other pthreads
    // are supplying MAX_EXTRA_HEAPCLEANER_ROOTS
    // in addition to the standard roots.
    //
    // This #define is referenced only in:
    //
    //     src/c/heapcleaner/call-heapcleaner.c   	
    //
    #define MAX_TOTAL_CLEANING_ROOTS	ROUND_UP_TO_POWER_OF_TWO(   MAX_PTHREADS    * (MAX_C_HEAPCLEANER_ROOTS + NROOTS + N_PSEUDO_ROOTS) +	\
						       (MAX_PTHREADS-1) * MAX_EXTRA_HEAPCLEANER_ROOTS +1,				\
						     8 )
#else
    #define MAX_TOTAL_CLEANING_ROOTS	ROUND_UP_TO_POWER_OF_TWO( MAX_PTHREADS * (MAX_C_HEAPCLEANER_ROOTS + NROOTS + N_PSEUDO_ROOTS) +1, 8)
#endif

#if NEED_SOFTWARE_GENERATED_PERIODIC_EVENTS  
    //
    // This #define is referenced only in:
    //
    //     src/c/heapcleaner/call-heapcleaner.c
    //     src/c/heapcleaner/pthread-heapcleaner-stuff.c
    //
    #define PERIODIC_EVENT_TIME_GRANULARITY_IN_NEXTCODE_INSTRUCTIONS   (1 << 10)     // Must be power of 2.
#endif

#endif // RUNTIME_CONFIGURATION_H



// COPYRIGHT (c) 1992 AT&T Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


