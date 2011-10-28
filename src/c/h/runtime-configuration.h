// runtime-configuration.h
//
// Various limits and default settings for the Lib7 run-time system.


#ifndef RUNTIME_CONFIGURATION_H
#define RUNTIME_CONFIGURATION_H

#include "runtime-base.h"

// Default image: NULL (means: try to find in-core image using dlopen/dlsym)
//
#ifndef DEFAULT_IMAGE
#define DEFAULT_IMAGE		NULL
#endif

// Maximum length of a boot-file pathname:
//
#ifndef     MAX_BOOT_PATH_LEN
    #define MAX_BOOT_PATH_LEN	512
#endif

// Mmaximum number of boot files
//
#ifndef     MAX_NUM_BOOT_FILES
    #define MAX_NUM_BOOT_FILES	1024
#endif

// Multicore support limits:
//
#ifdef MULTICORE_SUPPORT
    #ifndef MAX_PTHREADS
        #define MAX_PTHREADS	8
    #endif
#else
    #define MAX_PTHREADS	1
#endif


// Default heap sizes:
//
#ifndef     DEFAULT_ACTIVE_AGEGROUPS
    #define DEFAULT_ACTIVE_AGEGROUPS	5
#endif

#define MAX_ACTIVE_AGEGROUPS	14								// Should agree with MAX_AGEGROUPS in  sibid.h.
#define DEFAULT_OLDEST_AGEGROUP_KEEPING_IDLE_FROMSPACE_BUFFERS	2			// Keep idle fromspace regions for ages 1 & 2

#ifndef     DEFAULT_AGEGROUP0_BUFFER_BYTESIZE
    #define DEFAULT_AGEGROUP0_BUFFER_BYTESIZE	(256 * ONE_K_BINARY)
#endif

#ifdef OLD_POLICY
    #define RATIO_UNIT	16				// Ratios are measured in 1/16ths.
    #define DEFAULT_RATIO1	(7*(RATIO_UNIT/2))	// agegroup-1 sib buffers are small.
    #define DEFAULT_RATIO	(3*RATIO_UNIT)
    #define MAX_SZ1(NSZ)	(5*(NSZ))
#endif
#define DEFAULT_RATIO1	20
#define DEFAULT_RATIO2	10
#define DEFAULT_RATIO	5
#define MAX_SZ1(NSZ)	(6*(NSZ))

// Agegroup in which to allocate code chunks:
//
#define CODECHUNK_ALLOCATION_AGEGROUP	2

// Size-in-words of a "small chunk."
// The C allocation routines allocate
// small chunks in agegroup0,
// while large chunks are allocated in
// agegroup 1.
//
#define MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS	512

// Size-in-bytes of the allocation buffer.
// If A is the value of the limit pointer,
// then A[ALLOCATION_BUFFER_SIZE_IN_WORDS-1] is the address of
// the next store-vector location.
//
#define ALLOCATION_BUFFER_SIZE_IN_WORDS	  (1024 + 128)							// Referenced only on next line.
#define ALLOCATION_BUFFER_BYTESIZE	  (ALLOCATION_BUFFER_SIZE_IN_WORDS*WORD_BYTESIZE)		// Referenced only in   src/c/h/heap.h

// Maximum number of global C variables
// that can be garbage collection roots.
//
#define  MAX_C_CLEANING_ROOTS	16

// Maximum number of additional roots
// that can be passed to cleaner:
//
#define MAX_EXTRA_CLEANING_ROOTS 16

// Number of potential cleaner roots.
// This includes space for C global roots,
// Mythryl roots and the terminating null pointer.
//
#ifdef N_PSEUDO_REGS
    #define N_PSEUDO_ROOTS	N_PSEUDO_REGS
#else
    #define N_PSEUDO_ROOTS	0
#endif

#ifdef MULTICORE_SUPPORT
    // 
    // We must assume that all other pthreads
    // are supplying MAX_EXTRA_CLEANING_ROOTS
    // in addition to the standard roots.
    //
    // This #define is referenced only in:
    //
    //     src/c/heapcleaner/call-cleaner.c   	
    //
    #define MAX_TOTAL_CLEANING_ROOTS	ROUND_UP_TO_POWER_OF_TWO(   MAX_PTHREADS    * (MAX_C_CLEANING_ROOTS + NROOTS + N_PSEUDO_ROOTS) +	\
						       (MAX_PTHREADS-1) * MAX_EXTRA_CLEANING_ROOTS +1,				\
						     8 )
#else
    #define MAX_TOTAL_CLEANING_ROOTS	ROUND_UP_TO_POWER_OF_TWO( MAX_PTHREADS * (MAX_C_CLEANING_ROOTS + NROOTS + N_PSEUDO_ROOTS) +1, 8)
#endif

#ifdef SOFTWARE_GENERATED_PERIODIC_EVENTS  
    //
    // This #define is referenced only in:
    //
    //     src/c/heapcleaner/call-cleaner.c
    //     src/c/heapcleaner/multicore-cleaning-stuff.c
    //
    #define PERIODIC_EVENT_TIME_GRANULARITY_IN_NEXTCODE_INSTRUCTIONS   (1 << 10)     // Must be power of 2.
#endif

#endif // RUNTIME_CONFIGURATION_H



// COPYRIGHT (c) 1992 AT&T Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


